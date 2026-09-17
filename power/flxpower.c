/*
 * FreeLinX Power & Hardware Management Utility (flxpower)
 *
 * Lightweight, non-GNU hardware power, battery, brightness, and governor utility.
 * Reads directly from Linux sysfs (/sys/class/power_supply, /sys/class/backlight,
 * /sys/devices/system/cpu/cpufreq, /sys/class/thermal).
 *
 * Copyright (c) 2026 FreeLinX OS Project.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BLUE    "\033[1;34m"

static int read_sysfs_string(const char *path, char *buf, size_t len) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, len - 1);
    close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    /* Trim trailing newline */
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
        buf[--n] = '\0';
    }
    return 0;
}

static long read_sysfs_long(const char *path) {
    char buf[64];
    if (read_sysfs_string(path, buf, sizeof(buf)) < 0) return -1;
    return strtol(buf, NULL, 10);
}

static int write_sysfs_string(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    ssize_t len = strlen(val);
    ssize_t n = write(fd, val, len);
    close(fd);
    return (n == len) ? 0 : -1;
}

/* Print battery and AC status */
static void show_power_supply(void) {
    DIR *dir = opendir("/sys/class/power_supply");
    if (!dir) {
        printf("  Power Supply: (sysfs power_supply not available)\n");
        return;
    }

    struct dirent *entry;
    int found_battery = 0;
    int found_ac = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/power_supply/%s/type", entry->d_name);
        char type[32];
        if (read_sysfs_string(path, type, sizeof(type)) < 0) continue;

        if (strcmp(type, "Mains") == 0 || strcmp(type, "USB") == 0) {
            snprintf(path, sizeof(path), "/sys/class/power_supply/%s/online", entry->d_name);
            long online = read_sysfs_long(path);
            printf("  AC Adapter (%s): %s%s%s\n", entry->d_name,
                   online == 1 ? COLOR_GREEN "Connected (Online)" : COLOR_YELLOW "Disconnected (Offline)",
                   "", COLOR_RESET);
            found_ac = 1;
        } else if (strcmp(type, "Battery") == 0) {
            char status[32] = "Unknown";
            snprintf(path, sizeof(path), "/sys/class/power_supply/%s/status", entry->d_name);
            read_sysfs_string(path, status, sizeof(status));

            snprintf(path, sizeof(path), "/sys/class/power_supply/%s/capacity", entry->d_name);
            long capacity = read_sysfs_long(path);

            /* Energy / Power metrics */
            snprintf(path, sizeof(path), "/sys/class/power_supply/%s/power_now", entry->d_name);
            long power_now = read_sysfs_long(path);
            if (power_now < 0) {
                snprintf(path, sizeof(path), "/sys/class/power_supply/%s/current_now", entry->d_name);
                power_now = read_sysfs_long(path);
            }

            const char *col = COLOR_GREEN;
            if (capacity <= 15) col = COLOR_RED;
            else if (capacity <= 30) col = COLOR_YELLOW;

            printf("  Battery (%s): %s%ld%%%s [%s]",
                   entry->d_name, col, capacity >= 0 ? capacity : 0, COLOR_RESET, status);

            if (power_now > 0) {
                double watts = (double)power_now / 1000000.0;
                printf(" (%.2f W)", watts);
            }
            printf("\n");
            found_battery = 1;
        }
    }
    closedir(dir);

    if (!found_battery && !found_ac) {
        printf("  Power Supply: Desktop / Virtual Machine (No battery detected)\n");
    }
}

/* Backlight control and inspection */
static void show_backlight(void) {
    DIR *dir = opendir("/sys/class/backlight");
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/backlight/%s/brightness", entry->d_name);
        long cur = read_sysfs_long(path);
        snprintf(path, sizeof(path), "/sys/class/backlight/%s/max_brightness", entry->d_name);
        long max = read_sysfs_long(path);

        if (cur >= 0 && max > 0) {
            int pct = (int)((cur * 100) / max);
            printf("  Backlight (%s): %s%d%%%s (%ld/%ld)\n",
                   entry->d_name, COLOR_CYAN, pct, COLOR_RESET, cur, max);
        }
    }
    closedir(dir);
}

static int set_backlight(const char *arg) {
    DIR *dir = opendir("/sys/class/backlight");
    if (!dir) {
        fprintf(stderr, "Error: No backlight device found in /sys/class/backlight.\n");
        return 1;
    }

    struct dirent *entry;
    char bl_name[128] = "";
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            strncpy(bl_name, entry->d_name, sizeof(bl_name) - 1);
            break;
        }
    }
    closedir(dir);

    if (bl_name[0] == '\0') {
        fprintf(stderr, "Error: No backlight controller found.\n");
        return 1;
    }

    char path_cur[256], path_max[256];
    snprintf(path_cur, sizeof(path_cur), "/sys/class/backlight/%s/brightness", bl_name);
    snprintf(path_max, sizeof(path_max), "/sys/class/backlight/%s/max_brightness", bl_name);

    long cur = read_sysfs_long(path_cur);
    long max = read_sysfs_long(path_max);
    if (cur < 0 || max <= 0) {
        fprintf(stderr, "Error reading backlight bounds for %s.\n", bl_name);
        return 1;
    }

    long target = cur;
    if (strchr(arg, '%')) {
        long pct = strtol(arg, NULL, 10);
        if (pct < 1) pct = 1;
        if (pct > 100) pct = 100;
        target = (pct * max) / 100;
    } else if (arg[0] == '+') {
        long delta = strtol(arg + 1, NULL, 10);
        target = cur + delta;
    } else if (arg[0] == '-') {
        long delta = strtol(arg + 1, NULL, 10);
        target = cur - delta;
    } else {
        target = strtol(arg, NULL, 10);
    }

    if (target < 1) target = 1;
    if (target > max) target = max;

    char val_str[32];
    snprintf(val_str, sizeof(val_str), "%ld", target);
    if (write_sysfs_string(path_cur, val_str) < 0) {
        fprintf(stderr, "Error writing brightness to %s (permission denied?)\n", path_cur);
        return 1;
    }

    printf("Backlight [%s]: %ld/%ld (%d%%)\n", bl_name, target, max, (int)((target * 100) / max));
    return 0;
}

/* CPU frequency and thermal */
static void show_cpu_and_thermal(void) {
    /* CPU Governor & Frequency */
    char gov[64] = "unknown";
    read_sysfs_string("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", gov, sizeof(gov));
    long freq_khz = read_sysfs_long("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");

    if (freq_khz > 0) {
        printf("  CPU Governor: %s%s%s @ %.2f GHz\n",
               COLOR_BLUE, gov, COLOR_RESET, (double)freq_khz / 1000000.0);
    } else {
        printf("  CPU Governor: %s%s%s\n", COLOR_BLUE, gov, COLOR_RESET);
    }

    /* Thermal Zone */
    DIR *dir = opendir("/sys/class/thermal");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
                char path[256];
                snprintf(path, sizeof(path), "/sys/class/thermal/%s/temp", entry->d_name);
                long temp_mc = read_sysfs_long(path);
                if (temp_mc > 0) {
                    double deg = (double)temp_mc / 1000.0;
                    const char *col = COLOR_GREEN;
                    if (deg > 75.0) col = COLOR_RED;
                    else if (deg > 60.0) col = COLOR_YELLOW;
                    printf("  Thermal (%s): %s%.1f °C%s\n", entry->d_name, col, deg, COLOR_RESET);
                    break;
                }
            }
        }
        closedir(dir);
    }
}

static int set_governor(const char *gov) {
    int updated = 0;
    for (int cpu = 0; cpu < 128; cpu++) {
        char path[256];
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", cpu);
        if (access(path, F_OK) != 0) break;
        if (write_sysfs_string(path, gov) == 0) {
            updated++;
        }
    }
    if (updated == 0) {
        fprintf(stderr, "Error: Could not update CPU scaling governor.\n");
        return 1;
    }
    printf("CPU Governor set to '%s' on %d cores.\n", gov, updated);
    return 0;
}

static void show_battery_raw(void) {
    DIR *dir = opendir("/sys/class/power_supply");
    if (!dir) {
        printf("NONE 0\n");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char path[256];
        snprintf(path, sizeof(path), "/sys/class/power_supply/%s/type", entry->d_name);
        char type[32];
        if (read_sysfs_string(path, type, sizeof(type)) == 0 && strcmp(type, "Battery") == 0) {
            char status[32] = "Unknown";
            snprintf(path, sizeof(path), "/sys/class/power_supply/%s/status", entry->d_name);
            read_sysfs_string(path, status, sizeof(status));

            snprintf(path, sizeof(path), "/sys/class/power_supply/%s/capacity", entry->d_name);
            long capacity = read_sysfs_long(path);

            printf("%s %ld\n", status, capacity >= 0 ? capacity : 0);
            closedir(dir);
            return;
        }
    }
    closedir(dir);
    printf("AC 100\n");
}

static void print_usage(const char *prog) {
    printf("FreeLinX Power Management Utility (flxpower)\n");
    printf("Usage: %s [command] [args...]\n\n", prog);
    printf("Commands:\n");
    printf("  status (or no args)      Display complete power, battery & thermal state\n");
    printf("  brightness <val|%%|+val>   Set or adjust screen brightness (e.g. 50%%, +10, -10)\n");
    printf("  governor <gov>           Set CPU governor (performance, powersave, schedutil)\n");
    printf("  battery                  Output status for statusbar / scripts\n");
    printf("  suspend                  Suspend system to RAM (sleep)\n");
    printf("  help                     Show this help message\n");
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "status") == 0) {
        printf("%s%s=== FreeLinX Hardware & Power Status ===%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
        show_power_supply();
        show_backlight();
        show_cpu_and_thermal();
        return 0;
    }

    if (strcmp(argv[1], "brightness") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s brightness <value|%%|+val|-val>\n", argv[0]);
            return 1;
        }
        return set_backlight(argv[2]);
    }

    if (strcmp(argv[1], "governor") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s governor <powersave|performance|schedutil>\n", argv[0]);
            return 1;
        }
        return set_governor(argv[2]);
    }

    if (strcmp(argv[1], "battery") == 0) {
        show_battery_raw();
        return 0;
    }

    if (strcmp(argv[1], "suspend") == 0) {
        printf("Suspending system to RAM (mem)...\n");
        sync();
        if (write_sysfs_string("/sys/power/state", "mem") < 0) {
            fprintf(stderr, "Error triggering suspend (requires root/doas).\n");
            return 1;
        }
        return 0;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    fprintf(stderr, "Unknown command '%s'. Run '%s help' for usage.\n", argv[1], argv[0]);
    return 1;
}
