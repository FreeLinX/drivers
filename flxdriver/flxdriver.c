/*
 * FreeLinX Driver Manager (flxdriver)
 *
 * Copyright (c) 2026 FreeLinX Project
 * BSD 2-Clause License
 *
 * Scans hardware (PCI/USB), detects devices, probes and loads kernel modules,
 * and monitors audio, graphics, and network hardware without GNU dependencies.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <errno.h>

#define VERSION "1.0.0"

/* Helper to read a single line or trimmed string from a sysfs file */
static int read_sysfs_string(const char *path, char *buf, size_t maxlen) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, maxlen - 1);
    close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    /* Strip trailing newline/spaces */
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
        buf[--n] = '\0';
    }
    return 0;
}

/* Helper to get driver name from driver symlink in sysfs */
static int get_driver_name(const char *dev_path, char *buf, size_t maxlen) {
    char link_path[1024];
    char target[1024];
    snprintf(link_path, sizeof(link_path), "%s/driver", dev_path);
    ssize_t len = readlink(link_path, target, sizeof(target) - 1);
    if (len < 0) return -1;
    target[len] = '\0';
    char *base = strrchr(target, '/');
    if (base) base++;
    else base = target;
    strncpy(buf, base, maxlen - 1);
    buf[maxlen - 1] = '\0';
    return 0;
}

/* Decode PCI Class Code into human-readable description */
static const char *decode_pci_class(unsigned int class_code) {
    unsigned int base_class = (class_code >> 16) & 0xff;
    unsigned int sub_class  = (class_code >> 8) & 0xff;

    switch (base_class) {
        case 0x01: /* Mass Storage */
            if (sub_class == 0x01) return "IDE Controller";
            if (sub_class == 0x06) return "SATA AHCI Controller";
            if (sub_class == 0x08) return "NVMe Storage Controller";
            return "Mass Storage Controller";
        case 0x02: /* Network */
            if (sub_class == 0x00) return "Ethernet Controller";
            if (sub_class == 0x80) return "Wireless/Network Controller";
            return "Network Controller";
        case 0x03: /* Display */
            if (sub_class == 0x00) return "VGA Display Controller (GPU)";
            return "Display Controller";
        case 0x04: /* Multimedia */
            if (sub_class == 0x01 || sub_class == 0x03) return "Audio / Sound Controller";
            return "Multimedia Controller";
        case 0x06: /* Bridge */
            if (sub_class == 0x00) return "Host Bridge";
            if (sub_class == 0x01) return "ISA Bridge";
            if (sub_class == 0x04) return "PCI-to-PCI Bridge";
            return "Bridge Device";
        case 0x07: return "Communication Controller";
        case 0x08: return "Generic System Peripheral";
        case 0x0c: /* Serial Bus */
            if (sub_class == 0x03) return "USB Host Controller";
            if (sub_class == 0x05) return "SMBus Controller";
            return "Serial Bus Controller";
        case 0x0d: return "Wireless Controller";
        default:   return "Other Device";
    }
}

/* Execute modprobe or insmod for a given module/modalias */
static int run_modprobe(const char *arg) {
    pid_t pid = fork();
    if (pid == 0) {
        /* Child */
        int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
            close(null_fd);
        }
        execl("/sbin/modprobe", "modprobe", "-q", arg, (char *)NULL);
        /* Fallback to modprobe in PATH */
        execlp("modprobe", "modprobe", "-q", arg, (char *)NULL);
        _exit(127);
    }
    if (pid < 0) return -1;
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

/* Subcommand: scan */
static int cmd_scan(void) {
    printf("\033[1;36m=== FreeLinX Hardware & Driver Inventory ===\033[0m\n\n");

    /* PCI Devices */
    printf("\033[1;33m[ PCI Devices ]\033[0m\n");
    printf("%-14s %-10s %-28s %-16s %s\n", "BUS ID", "VENDOR:DEV", "CLASS / TYPE", "DRIVER", "STATUS");
    printf("----------------------------------------------------------------------------------------\n");

    DIR *dir = opendir("/sys/bus/pci/devices");
    if (dir) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') continue;

            char path[1024];
            snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s", de->d_name);

            char vendor[64] = "unknown";
            char device[64] = "unknown";
            char class_str[64] = "0";
            char driver[64] = "-";
            int has_driver = 0;

            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s/vendor", path);
            read_sysfs_string(tmp, vendor, sizeof(vendor));

            snprintf(tmp, sizeof(tmp), "%s/device", path);
            read_sysfs_string(tmp, device, sizeof(device));

            snprintf(tmp, sizeof(tmp), "%s/class", path);
            read_sysfs_string(tmp, class_str, sizeof(class_str));

            if (get_driver_name(path, driver, sizeof(driver)) == 0) {
                has_driver = 1;
            }

            unsigned int class_code = (unsigned int)strtoul(class_str, NULL, 16);
            const char *class_desc = decode_pci_class(class_code);

            char id_pair[32];
            /* Strip leading 0x */
            const char *v = (strncmp(vendor, "0x", 2) == 0) ? vendor + 2 : vendor;
            const char *d = (strncmp(device, "0x", 2) == 0) ? device + 2 : device;
            snprintf(id_pair, sizeof(id_pair), "%s:%s", v, d);

            printf("%-14s %-10s %-28s %-16s %s\n",
                   de->d_name,
                   id_pair,
                   class_desc,
                   driver,
                   has_driver ? "\033[32m[Active]\033[0m" : "\033[31m[Unbound]\033[0m");
        }
        closedir(dir);
    } else {
        printf("  (PCI bus not found / /sys not mounted)\n");
    }

    printf("\n\033[1;33m[ USB Devices ]\033[0m\n");
    printf("%-10s %-12s %-26s %-16s %s\n", "DEV ID", "VENDOR:PROD", "PRODUCT / NAME", "DRIVER", "STATUS");
    printf("----------------------------------------------------------------------------------------\n");

    dir = opendir("/sys/bus/usb/devices");
    if (dir) {
        struct dirent *de;
        int usb_count = 0;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') continue;

            char path[1024];
            snprintf(path, sizeof(path), "/sys/bus/usb/devices/%s", de->d_name);

            char idVendor[32] = "";
            char idProduct[32] = "";
            char product[128] = "USB Device";
            char driver[64] = "-";
            int has_driver = 0;

            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s/idVendor", path);
            if (read_sysfs_string(tmp, idVendor, sizeof(idVendor)) != 0) {
                continue; /* Interface, not root device */
            }

            snprintf(tmp, sizeof(tmp), "%s/idProduct", path);
            read_sysfs_string(tmp, idProduct, sizeof(idProduct));

            snprintf(tmp, sizeof(tmp), "%s/product", path);
            read_sysfs_string(tmp, product, sizeof(product));

            if (get_driver_name(path, driver, sizeof(driver)) == 0) {
                has_driver = 1;
            }

            char id_pair[32];
            snprintf(id_pair, sizeof(id_pair), "%s:%s", idVendor, idProduct);

            printf("%-10s %-12s %-26.26s %-16s %s\n",
                   de->d_name,
                   id_pair,
                   product,
                   driver,
                   has_driver ? "\033[32m[Active]\033[0m" : "\033[33m[Interface]\033[0m");
            usb_count++;
        }
        closedir(dir);
        if (usb_count == 0) {
            printf("  (No USB devices found)\n");
        }
    } else {
        printf("  (USB bus not found)\n");
    }

    printf("\n");
    return 0;
}

/* Subcommand: probe (auto-loads kernel modules for unbound hardware) */
static int cmd_probe(void) {
    printf("\033[1;36m=== FreeLinX Auto-Probing Drivers ===\033[0m\n");

    int probed = 0;
    int success = 0;

    /* Check PCI devices */
    DIR *dir = opendir("/sys/bus/pci/devices");
    if (dir) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') continue;

            char path[1024];
            snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s", de->d_name);

            char driver[64];
            if (get_driver_name(path, driver, sizeof(driver)) == 0) {
                /* Driver already bound */
                continue;
            }

            char modalias[256];
            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s/modalias", path);
            if (read_sysfs_string(tmp, modalias, sizeof(modalias)) == 0 && modalias[0] != '\0') {
                printf("[PCI %s] Probing: %s ... ", de->d_name, modalias);
                fflush(stdout);
                probed++;
                if (run_modprobe(modalias) == 0) {
                    printf("\033[32mOK\033[0m\n");
                    success++;
                } else {
                    printf("\033[33mNo module matched\033[0m\n");
                }
            }
        }
        closedir(dir);
    }

    /* Check USB devices */
    dir = opendir("/sys/bus/usb/devices");
    if (dir) {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (de->d_name[0] == '.') continue;

            char path[1024];
            snprintf(path, sizeof(path), "/sys/bus/usb/devices/%s", de->d_name);

            char driver[64];
            if (get_driver_name(path, driver, sizeof(driver)) == 0) {
                continue;
            }

            char modalias[256];
            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s/modalias", path);
            if (read_sysfs_string(tmp, modalias, sizeof(modalias)) == 0 && modalias[0] != '\0') {
                printf("[USB %s] Probing: %s ... ", de->d_name, modalias);
                fflush(stdout);
                probed++;
                if (run_modprobe(modalias) == 0) {
                    printf("\033[32mOK\033[0m\n");
                    success++;
                } else {
                    printf("\033[33mNo module matched\033[0m\n");
                }
            }
        }
        closedir(dir);
    }

    printf("\nAuto-probe finished. Total examined unbound devices: %d, Modules loaded: %d\n", probed, success);
    return 0;
}

/* Subcommand: audio */
static int cmd_audio(void) {
    printf("\033[1;36m=== FreeLinX Sound & Audio Subsystem ===\033[0m\n\n");

    /* 1. Check ALSA soundcards */
    FILE *f = fopen("/proc/asound/cards", "r");
    if (f) {
        printf("\033[1;32mDetected Audio Cards (/proc/asound/cards):\033[0m\n");
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            printf("  %s", line);
        }
        fclose(f);
    } else {
        printf("\033[33mNo audio cards registered in /proc/asound/cards\033[0m\n");
    }

    /* 2. Check /dev/snd endpoints */
    printf("\n\033[1;32mAudio Device Nodes (/dev/snd/):\033[0m\n");
    DIR *d = opendir("/dev/snd");
    if (d) {
        struct dirent *de;
        int count = 0;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.') continue;
            printf("  /dev/snd/%-16s ", de->d_name);
            if (strncmp(de->d_name, "pcm", 3) == 0) {
                if (de->d_name[strlen(de->d_name)-1] == 'p') {
                    printf("(Playback Stream)\n");
                } else if (de->d_name[strlen(de->d_name)-1] == 'c') {
                    printf("(Capture/Record Stream)\n");
                } else {
                    printf("(PCM Stream)\n");
                }
            } else if (strncmp(de->d_name, "control", 7) == 0) {
                printf("(Mixer / Volume Control)\n");
            } else if (strncmp(de->d_name, "timer", 5) == 0) {
                printf("(ALSA Timer)\n");
            } else {
                printf("\n");
            }
            count++;
        }
        closedir(d);
        if (count == 0) printf("  (None found)\n");
    } else {
        printf("  /dev/snd not found.\n");
    }

    /* 3. Check TinyALSA userspace tools */
    printf("\n\033[1;32mNon-GNU Audio Tools (TinyALSA):\033[0m\n");
    const char *tools[] = {"/usr/bin/tinymix", "/usr/bin/tinyplay", "/usr/bin/tinycap", "/usr/bin/tinypcminfo"};
    for (size_t i = 0; i < sizeof(tools)/sizeof(tools[0]); i++) {
        if (access(tools[i], X_OK) == 0) {
            printf("  %-24s \033[32m[Installed & Executable]\033[0m\n", tools[i]);
        } else {
            printf("  %-24s \033[31m[Missing]\033[0m\n", tools[i]);
        }
    }
    printf("\n");
    return 0;
}

/* Subcommand: net */
static int cmd_net(void) {
    printf("\033[1;36m=== FreeLinX Network & Wireless Interfaces ===\033[0m\n\n");
    printf("%-10s %-12s %-18s %-14s %s\n", "IFACE", "TYPE", "MAC ADDRESS", "DRIVER", "OPERSTATE");
    printf("----------------------------------------------------------------------\n");

    DIR *d = opendir("/sys/class/net");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.') continue;

            char path[1024];
            snprintf(path, sizeof(path), "/sys/class/net/%s", de->d_name);

            char operstate[32] = "unknown";
            char address[32] = "-";
            char driver[64] = "-";
            char type[32] = "Ethernet";

            char tmp[1024];
            snprintf(tmp, sizeof(tmp), "%s/operstate", path);
            read_sysfs_string(tmp, operstate, sizeof(operstate));

            snprintf(tmp, sizeof(tmp), "%s/address", path);
            read_sysfs_string(tmp, address, sizeof(address));

            snprintf(tmp, sizeof(tmp), "%s/device", path);
            get_driver_name(tmp, driver, sizeof(driver));

            snprintf(tmp, sizeof(tmp), "%s/wireless", path);
            struct stat st;
            if (stat(tmp, &st) == 0) {
                snprintf(type, sizeof(type), "Wireless(802.11)");
            } else {
                snprintf(tmp, sizeof(tmp), "%s/phy80211", path);
                if (stat(tmp, &st) == 0) {
                    snprintf(type, sizeof(type), "Wireless(802.11)");
                } else if (strcmp(de->d_name, "lo") == 0) {
                    snprintf(type, sizeof(type), "Loopback");
                }
            }

            printf("%-10s %-12s %-18s %-14s %s\n",
                   de->d_name,
                   type,
                   address,
                   driver,
                   strcmp(operstate, "up") == 0 ? "\033[32mUP\033[0m" : operstate);
        }
        closedir(d);
    } else {
        printf("  /sys/class/net not available.\n");
    }
    printf("\n");
    return 0;
}

/* Subcommand: status */
static int cmd_status(void) {
    struct utsname un;
    uname(&un);
    printf("\033[1;36m=== FreeLinX Driver Subsystem Status ===\033[0m\n");
    printf("Kernel: %s %s (%s)\n\n", un.sysname, un.release, un.machine);

    /* 1. Kernel Modules */
    printf("\033[1;33m[ Loaded Kernel Modules (/proc/modules) ]\033[0m\n");
    FILE *f = fopen("/proc/modules", "r");
    if (f) {
        char line[256];
        int count = 0;
        while (fgets(line, sizeof(line), f)) {
            char name[64];
            unsigned long size;
            int refs;
            if (sscanf(line, "%63s %lu %d", name, &size, &refs) == 3) {
                printf("  %-24s %8lu bytes (refs: %d)\n", name, size, refs);
                count++;
            }
        }
        fclose(f);
        if (count == 0) printf("  (No loadable modules currently active)\n");
    } else {
        printf("  /proc/modules not available\n");
    }

    /* 2. Firmware Directory */
    printf("\n\033[1;33m[ Firmware Status (/lib/firmware) ]\033[0m\n");
    struct stat st;
    if (stat("/lib/firmware", &st) == 0 && S_ISDIR(st.st_mode)) {
        printf("  /lib/firmware: \033[32mPresent\033[0m\n");
        DIR *fd = opendir("/lib/firmware");
        if (fd) {
            struct dirent *de;
            int fw_dirs = 0;
            while ((de = readdir(fd)) != NULL) {
                if (de->d_name[0] == '.') continue;
                if (fw_dirs < 10) {
                    printf("    - %s\n", de->d_name);
                }
                fw_dirs++;
            }
            if (fw_dirs > 10) {
                printf("    ... and %d more firmware packages\n", fw_dirs - 10);
            }
            closedir(fd);
        }
    } else {
        printf("  /lib/firmware: \033[31mNot found\033[0m (Run 'flxfirmware install' to populate)\n");
    }

    /* 3. DRM / Graphics */
    printf("\n\033[1;33m[ Graphics / DRM Subsystem (/sys/class/drm) ]\033[0m\n");
    DIR *d = opendir("/sys/class/drm");
    if (d) {
        struct dirent *de;
        int drm_count = 0;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.') continue;
            if (strncmp(de->d_name, "card", 4) == 0 && strchr(de->d_name, '-') == NULL) {
                char dev_path[1024];
                snprintf(dev_path, sizeof(dev_path), "/sys/class/drm/%s/device", de->d_name);
                char driver[64] = "-";
                get_driver_name(dev_path, driver, sizeof(driver));
                printf("  Device: /dev/dri/%-10s Driver: %-16s \033[32m[Active]\033[0m\n", de->d_name, driver);
                drm_count++;
            }
        }
        closedir(d);
        if (drm_count == 0) printf("  (No hardware DRM acceleration card; software/framebuffer active)\n");
    } else {
        printf("  /sys/class/drm not available\n");
    }

    printf("\n");
    return 0;
}

/* Subcommand: gpu (3D hardware acceleration diagnostics) */
static int cmd_gpu(void) {
    printf("\033[1;36m=== FreeLinX GPU & 3D Acceleration Diagnostics ===\033[0m\n\n");

    /* 1. DRM Devices */
    printf("\033[1;33m[ Direct Rendering Manager (DRM / KMS) ]\033[0m\n");
    DIR *d = opendir("/sys/class/drm");
    int has_card = 0;
    int has_render = 0;
    char primary_driver[64] = "none";

    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.') continue;
            if (strncmp(de->d_name, "card", 4) == 0 && strchr(de->d_name, '-') == NULL) {
                char dev_path[1024];
                snprintf(dev_path, sizeof(dev_path), "/sys/class/drm/%s/device", de->d_name);
                char driver[64] = "-";
                get_driver_name(dev_path, driver, sizeof(driver));
                strncpy(primary_driver, driver, sizeof(primary_driver) - 1);
                printf("  Primary Display Card: /dev/dri/%-10s (Kernel Driver: \033[32m%s\033[0m)\n", de->d_name, driver);
                has_card++;
            } else if (strncmp(de->d_name, "renderD", 7) == 0) {
                char dev_path[1024];
                snprintf(dev_path, sizeof(dev_path), "/sys/class/drm/%s/device", de->d_name);
                char driver[64] = "-";
                get_driver_name(dev_path, driver, sizeof(driver));
                printf("  3D Hardware Render Node: /dev/dri/%-10s (Kernel Driver: \033[32m%s\033[0m)\n", de->d_name, driver);
                has_render++;
            }
        }
        closedir(d);
    }

    if (has_card == 0) {
        printf("  No KMS DRM card detected (Running pure software fbdev/shadowfb)\n");
    }

    /* 2. 3D Acceleration Evaluation */
    printf("\n\033[1;33m[ 3D Acceleration Evaluation ]\033[0m\n");
    if (has_render > 0) {
        printf("  3D Hardware Acceleration: \033[1;32mACTIVE\033[0m\n");
        if (strcmp(primary_driver, "virtio_gpu") == 0) {
            printf("  Architecture: \033[36mQEMU VirtIO 3D (VirGL 3D Acceleration enabled)\033[0m\n");
        } else if (strcmp(primary_driver, "i915") == 0 || strcmp(primary_driver, "xe") == 0) {
            printf("  Architecture: \033[36mIntel Gen Graphics (Direct Hardware 3D)\033[0m\n");
        } else if (strcmp(primary_driver, "amdgpu") == 0 || strcmp(primary_driver, "radeon") == 0) {
            printf("  Architecture: \033[36mAMD Radeon Graphics (Direct Hardware 3D)\033[0m\n");
        } else {
            printf("  Architecture: \033[36mGeneric DRM Hardware Render Node\033[0m\n");
        }
    } else {
        printf("  3D Hardware Acceleration: \033[33mINACTIVE / SOFTWARE ONLY\033[0m\n");
        printf("  Tip: For QEMU, run with 'GL=1 ./run.sh' to enable VirtIO 3D VirGL GPU.\n");
    }

    /* 3. Display Connectors */
    printf("\n\033[1;33m[ Display Outputs & Connectors ]\033[0m\n");
    d = opendir("/sys/class/drm");
    if (d) {
        struct dirent *de;
        int conn_count = 0;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.') continue;
            if (strchr(de->d_name, '-')) {
                char status_path[1024];
                snprintf(status_path, sizeof(status_path), "/sys/class/drm/%s/status", de->d_name);
                char status[32] = "unknown";
                read_sysfs_string(status_path, status, sizeof(status));
                printf("  %-20s Status: %s\n", de->d_name,
                       strcmp(status, "connected") == 0 ? "\033[32mconnected\033[0m" : "\033[37mdisconnected\033[0m");
                conn_count++;
            }
        }
        closedir(d);
        if (conn_count == 0) printf("  (No external DRM connectors listed)\n");
    }

    printf("\n");
    return 0;
}

static void print_usage(const char *prog) {
    printf("FreeLinX Driver Manager (flxdriver) v%s\n", VERSION);
    printf("Usage: %s <command> [options]\n\n", prog);
    printf("Commands:\n");
    printf("  scan, ls       Scan and list all PCI/USB hardware and driver bindings\n");
    printf("  probe, load    Auto-detect unbound hardware and modprobe matching drivers\n");
    printf("  status         Display overall driver, module, firmware, and graphics status\n");
    printf("  gpu, drm       Display GPU 3D acceleration and DRM render node diagnostics\n");
    printf("  audio          Show audio hardware endpoints (/dev/snd) and TinyALSA status\n");
    printf("  net            Show network interfaces (Ethernet / 802.11 WiFi) and state\n");
    printf("  version        Show version and license information\n");
    printf("  help           Show this help text\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "scan") == 0 || strcmp(cmd, "ls") == 0) {
        return cmd_scan();
    } else if (strcmp(cmd, "probe") == 0 || strcmp(cmd, "load") == 0) {
        return cmd_probe();
    } else if (strcmp(cmd, "status") == 0) {
        return cmd_status();
    } else if (strcmp(cmd, "gpu") == 0 || strcmp(cmd, "drm") == 0) {
        return cmd_gpu();
    } else if (strcmp(cmd, "audio") == 0) {
        return cmd_audio();
    } else if (strcmp(cmd, "net") == 0 || strcmp(cmd, "wifi") == 0) {
        return cmd_net();
    } else if (strcmp(cmd, "version") == 0 || strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        printf("flxdriver %s (FreeLinX no-GNU Hardware Subsystem)\nLicense: BSD-2-Clause\n", VERSION);
        return 0;
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\nRun '%s help' for usage.\n", cmd, argv[0]);
        return 1;
    }
}
