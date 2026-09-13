/*
 * FreeLinX GPT Partitioning Utility (flx-part)
 *
 * Fully autonomous, BSD-2-Clause GPT partition table creator and inspector.
 * Creates standard UEFI-compliant GPT tables with:
 *   - Protective MBR (LBA 0)
 *   - Primary GPT Header (LBA 1) & Entries (LBA 2..33)
 *   - ESP Partition (512MB, Type C12A7328-F81F-11D2-BA4B-00A0C93EC93B)
 *   - Root Partition (Remainder, Type 0FC63DAF-8483-4772-8E79-3D69D8477DE4)
 *   - Backup GPT Entries & Backup GPT Header
 *
 * Re-reads kernel partition table via ioctl(BLKRRPART).
 *
 * Copyright (c) 2026 FreeLinX OS Project.
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>

#define SECTOR_SIZE 512
#define GPT_ENTRIES 128
#define GPT_ENTRY_SIZE 128
#define GPT_ENTRY_SECTORS ((GPT_ENTRIES * GPT_ENTRY_SIZE) / SECTOR_SIZE) /* 32 */

/* UEFI GUID Structure */
typedef struct {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t  clock_seq_hi_and_reserved;
    uint8_t  clock_seq_low;
    uint8_t  node[6];
} __attribute__((packed)) guid_t;

/* MBR Partition Record */
typedef struct {
    uint8_t  boot_indicator;
    uint8_t  start_head;
    uint8_t  start_sector;
    uint8_t  start_cylinder;
    uint8_t  os_type;
    uint8_t  end_head;
    uint8_t  end_sector;
    uint8_t  end_cylinder;
    uint32_t starting_lba;
    uint32_t size_in_lba;
} __attribute__((packed)) mbr_entry_t;

/* Protective MBR */
typedef struct {
    uint8_t     boot_code[446];
    mbr_entry_t partitions[4];
    uint16_t    signature; /* 0xAA55 */
} __attribute__((packed)) mbr_t;

/* GPT Header */
typedef struct {
    uint64_t signature; /* "EFI PART" = 0x5452415020494645ULL */
    uint32_t revision;  /* 0x00010000 */
    uint32_t header_size; /* 92 bytes */
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    guid_t   disk_guid;
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t sizeof_partition_entry;
    uint32_t partition_entry_array_crc32;
    uint8_t  reserved_trailing[420]; /* Pad to 512 bytes */
} __attribute__((packed)) gpt_header_t;

/* GPT Partition Entry */
typedef struct {
    guid_t   type_guid;
    guid_t   unique_guid;
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t name[36]; /* UTF-16LE */
} __attribute__((packed)) gpt_entry_t;

/* Standard Type GUIDs */
/* EFI System Partition: C12A7328-F81F-11D2-BA4B-00A0C93EC93B */
static const guid_t GUID_ESP = {
    0xC12A7328, 0xF81F, 0x11D2, 0xBA, 0x4B, { 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }
};

/* Linux Generic Filesystem Data: 0FC63DAF-8483-4772-8E79-3D69D8477DE4 */
static const guid_t GUID_LINUX_ROOT = {
    0x0FC63DAF, 0x8483, 0x4772, 0x8E, 0x79, { 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4 }
};

/* CRC-32 (IEEE 802.3) */
static uint32_t crc32(const void *buf, size_t len) {
    static uint32_t table[256];
    static int have_table = 0;
    if (!have_table) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t rem = i;
            for (int j = 0; j < 8; j++) {
                if (rem & 1) rem = (rem >> 1) ^ 0xEDB88320;
                else rem >>= 1;
            }
            table[i] = rem;
        }
        have_table = 1;
    }

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p = (const uint8_t *)buf;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ table[(crc ^ p[i]) & 0xFF];
    }
    return ~crc;
}

/* Generate random v4 UUID */
static void generate_uuid(guid_t *guid) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, guid, sizeof(guid_t)) != sizeof(guid_t)) {
            memset(guid, 0x42, sizeof(guid_t));
        }
        close(fd);
    } else {
        /* Fallback */
        for (size_t i = 0; i < sizeof(guid_t); i++) {
            ((uint8_t *)guid)[i] = (uint8_t)rand();
        }
    }
    /* UUID v4 */
    guid->time_hi_and_version = (guid->time_hi_and_version & 0x0FFF) | 0x4000;
    guid->clock_seq_hi_and_reserved = (guid->clock_seq_hi_and_reserved & 0x3F) | 0x80;
}

static void format_guid(const guid_t *g, char *out, size_t out_len) {
    snprintf(out, out_len, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             g->time_low, g->time_mid, g->time_hi_and_version,
             g->clock_seq_hi_and_reserved, g->clock_seq_low,
             g->node[0], g->node[1], g->node[2], g->node[3], g->node[4], g->node[5]);
}

static void ascii_to_utf16le(const char *src, uint16_t *dst, size_t max_chars) {
    for (size_t i = 0; i < max_chars; i++) {
        if (src[i] == '\0') {
            dst[i] = 0;
            break;
        }
        dst[i] = (uint16_t)(uint8_t)src[i];
    }
}

static uint64_t get_device_size(int fd) {
    uint64_t bytes = 0;
    struct stat st;
    if (fstat(fd, &st) < 0) return 0;

    if (S_ISBLK(st.st_mode)) {
        if (ioctl(fd, BLKGETSIZE64, &bytes) == 0) {
            return bytes;
        }
    }
    /* File fallback */
    off_t pos = lseek(fd, 0, SEEK_END);
    if (pos > 0) return (uint64_t)pos;
    return 0;
}

static void print_usage(const char *prog) {
    printf("FreeLinX GPT Partitioning Utility (flx-part)\n");
    printf("Usage: %s [options] <device>\n", prog);
    printf("Options:\n");
    printf("  --create-standard    Create ESP (512MB) + Linux Root (remainder)\n");
    printf("  --esp-size <MB>      Set ESP partition size in MB (default: 512)\n");
    printf("  --show               Show partition table of device\n");
    printf("  --help               Display this help message\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *device = NULL;
    int opt_create = 0;
    int opt_show = 0;
    uint64_t esp_size_mb = 512;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--create-standard") == 0) {
            opt_create = 1;
        } else if (strcmp(argv[i], "--show") == 0) {
            opt_show = 1;
        } else if (strcmp(argv[i], "--esp-size") == 0 && i + 1 < argc) {
            esp_size_mb = strtoull(argv[++i], NULL, 10);
            if (esp_size_mb < 64) esp_size_mb = 64;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            device = argv[i];
        }
    }

    if (!device) {
        fprintf(stderr, "Error: No target device specified.\n");
        return 1;
    }

    int flags = opt_create ? O_RDWR : O_RDONLY;
    int fd = open(device, flags);
    if (fd < 0) {
        fprintf(stderr, "Error opening '%s': %s\n", device, strerror(errno));
        return 1;
    }

    uint64_t disk_bytes = get_device_size(fd);
    if (disk_bytes < (uint64_t)1024 * 1024 * 1024) { /* At least 1GB */
        fprintf(stderr, "Error: Device '%s' size (%.2f MB) is too small (minimum 1GB).\n",
                device, (double)disk_bytes / (1024.0 * 1024.0));
        close(fd);
        return 1;
    }

    uint64_t total_sectors = disk_bytes / SECTOR_SIZE;

    if (opt_show) {
        printf("Device: %s\n", device);
        printf("Size: %.2f GB (%llu sectors, 512 bytes/sector)\n\n",
               (double)disk_bytes / (1024.0 * 1024.0 * 1024.0),
               (unsigned long long)total_sectors);

        gpt_header_t hdr;
        if (pread(fd, &hdr, sizeof(hdr), SECTOR_SIZE) == sizeof(hdr)) {
            if (hdr.signature == 0x5452415020494645ULL) {
                char disk_uuid_str[64];
                format_guid(&hdr.disk_guid, disk_uuid_str, sizeof(disk_uuid_str));
                printf("GPT Partition Table detected!\n");
                printf("Disk GUID: %s\n", disk_uuid_str);
                printf("First Usable LBA: %llu, Last Usable LBA: %llu\n\n",
                       (unsigned long long)hdr.first_usable_lba,
                       (unsigned long long)hdr.last_usable_lba);

                gpt_entry_t entries[GPT_ENTRIES];
                if (pread(fd, entries, sizeof(entries), 2 * SECTOR_SIZE) == sizeof(entries)) {
                    printf("%-4s %-12s %-12s %-10s %-38s %s\n",
                           "No", "Start LBA", "End LBA", "Size", "PARTUUID", "Name");
                    printf("----------------------------------------------------------------------------------------\n");
                    for (int i = 0; i < GPT_ENTRIES; i++) {
                        if (entries[i].starting_lba == 0 && entries[i].ending_lba == 0) continue;
                        char part_uuid_str[64];
                        format_guid(&entries[i].unique_guid, part_uuid_str, sizeof(part_uuid_str));
                        char name[37];
                        for (int j = 0; j < 36; j++) {
                            name[j] = (char)entries[i].name[j];
                        }
                        name[36] = '\0';
                        uint64_t size_mb = ((entries[i].ending_lba - entries[i].starting_lba + 1) * SECTOR_SIZE) / (1024 * 1024);
                        printf("%-4d %-12llu %-12llu %llu MB    %-38s %s\n",
                               i + 1,
                               (unsigned long long)entries[i].starting_lba,
                               (unsigned long long)entries[i].ending_lba,
                               (unsigned long long)size_mb,
                               part_uuid_str, name);
                    }
                }
                close(fd);
                return 0;
            }
        }
        printf("No valid GPT partition table found on %s.\n", device);
        close(fd);
        return 0;
    }

    if (!opt_create) {
        print_usage(argv[0]);
        close(fd);
        return 1;
    }

    printf("=== Initializing GPT on %s (%.2f GB) ===\n",
           device, (double)disk_bytes / (1024.0 * 1024.0 * 1024.0));

    /* 1. Construct Protective MBR */
    mbr_t mbr;
    memset(&mbr, 0, sizeof(mbr));
    mbr.partitions[0].boot_indicator = 0x00;
    mbr.partitions[0].start_head = 0x00;
    mbr.partitions[0].start_sector = 0x02;
    mbr.partitions[0].start_cylinder = 0x00;
    mbr.partitions[0].os_type = 0xEE; /* GPT Protective */
    mbr.partitions[0].end_head = 0xFF;
    mbr.partitions[0].end_sector = 0xFF;
    mbr.partitions[0].end_cylinder = 0xFF;
    mbr.partitions[0].starting_lba = 1;
    mbr.partitions[0].size_in_lba = (total_sectors - 1 > 0xFFFFFFFF) ? 0xFFFFFFFF : (uint32_t)(total_sectors - 1);
    mbr.signature = 0xAA55;

    /* 2. Construct Partition Entries Array */
    gpt_entry_t entries[GPT_ENTRIES];
    memset(entries, 0, sizeof(entries));

    uint64_t esp_sectors = (esp_size_mb * 1024 * 1024) / SECTOR_SIZE;
    uint64_t esp_start = 2048; /* 1MB boundary */
    uint64_t esp_end = esp_start + esp_sectors - 1;

    uint64_t last_usable = total_sectors - 34;
    uint64_t root_start = esp_end + 1;
    uint64_t root_end = last_usable;

    /* Partition 1: ESP */
    entries[0].type_guid = GUID_ESP;
    generate_uuid(&entries[0].unique_guid);
    entries[0].starting_lba = esp_start;
    entries[0].ending_lba = esp_end;
    entries[0].attributes = 0;
    ascii_to_utf16le("EFI System Partition", entries[0].name, 36);

    /* Partition 2: Linux Root */
    entries[1].type_guid = GUID_LINUX_ROOT;
    generate_uuid(&entries[1].unique_guid);
    entries[1].starting_lba = root_start;
    entries[1].ending_lba = root_end;
    entries[1].attributes = 0;
    ascii_to_utf16le("FreeLinX Root", entries[1].name, 36);

    uint32_t entries_crc = crc32(entries, sizeof(entries));

    /* 3. Construct Primary GPT Header */
    gpt_header_t primary_hdr;
    memset(&primary_hdr, 0, sizeof(primary_hdr));
    primary_hdr.signature = 0x5452415020494645ULL; /* "EFI PART" */
    primary_hdr.revision = 0x00010000;
    primary_hdr.header_size = 92;
    primary_hdr.current_lba = 1;
    primary_hdr.backup_lba = total_sectors - 1;
    primary_hdr.first_usable_lba = 34;
    primary_hdr.last_usable_lba = last_usable;
    generate_uuid(&primary_hdr.disk_guid);
    primary_hdr.partition_entry_lba = 2;
    primary_hdr.num_partition_entries = GPT_ENTRIES;
    primary_hdr.sizeof_partition_entry = GPT_ENTRY_SIZE;
    primary_hdr.partition_entry_array_crc32 = entries_crc;
    primary_hdr.header_crc32 = crc32(&primary_hdr, 92);

    /* 4. Construct Backup GPT Header */
    gpt_header_t backup_hdr = primary_hdr;
    backup_hdr.current_lba = total_sectors - 1;
    backup_hdr.backup_lba = 1;
    backup_hdr.partition_entry_lba = total_sectors - 33;
    backup_hdr.header_crc32 = 0;
    backup_hdr.header_crc32 = crc32(&backup_hdr, 92);

    /* 5. Write to Disk */
    /* Sector 0: Protective MBR */
    if (pwrite(fd, &mbr, sizeof(mbr), 0) != sizeof(mbr)) {
        fprintf(stderr, "Error writing MBR: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    /* Sector 1: Primary GPT Header */
    if (pwrite(fd, &primary_hdr, sizeof(primary_hdr), SECTOR_SIZE) != sizeof(primary_hdr)) {
        fprintf(stderr, "Error writing Primary GPT Header: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    /* Sectors 2..33: Primary Partition Entries */
    if (pwrite(fd, entries, sizeof(entries), 2 * SECTOR_SIZE) != sizeof(entries)) {
        fprintf(stderr, "Error writing Partition Entries: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    /* Sectors (total_sectors - 33)..(total_sectors - 2): Backup Partition Entries */
    if (pwrite(fd, entries, sizeof(entries), (total_sectors - 33) * SECTOR_SIZE) != sizeof(entries)) {
        fprintf(stderr, "Error writing Backup Partition Entries: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    /* Sector (total_sectors - 1): Backup GPT Header */
    if (pwrite(fd, &backup_hdr, sizeof(backup_hdr), (total_sectors - 1) * SECTOR_SIZE) != sizeof(backup_hdr)) {
        fprintf(stderr, "Error writing Backup GPT Header: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    fsync(fd);

    /* Notify Kernel */
    printf("Informing kernel of new partition layout...\n");
    if (ioctl(fd, BLKRRPART, 0) < 0) {
        /* Not fatal on image files or when partitions busy */
        if (errno != ENOTTY && errno != EINVAL) {
            fprintf(stderr, "Notice: BLKRRPART ioctl: %s (run partprobe if needed)\n", strerror(errno));
        }
    }
    close(fd);

    char esp_uuid[64], root_uuid[64];
    format_guid(&entries[0].unique_guid, esp_uuid, sizeof(esp_uuid));
    format_guid(&entries[1].unique_guid, root_uuid, sizeof(root_uuid));

    /* Print shell-parseable output variables for the installer script */
    printf("\n[SUCCESS] GPT Partition Table written successfully!\n");
    printf("FLX_PART1_START=%llu\n", (unsigned long long)esp_start);
    printf("FLX_PART1_END=%llu\n", (unsigned long long)esp_end);
    printf("FLX_PART1_SIZE_MB=%llu\n", (unsigned long long)esp_size_mb);
    printf("FLX_PART1_UUID=%s\n", esp_uuid);
    printf("FLX_PART2_START=%llu\n", (unsigned long long)root_start);
    printf("FLX_PART2_END=%llu\n", (unsigned long long)root_end);
    printf("FLX_PART2_SIZE_MB=%llu\n", (unsigned long long)(((root_end - root_start + 1) * SECTOR_SIZE) / (1024 * 1024)));
    printf("FLX_PART2_UUID=%s\n", root_uuid);

    return 0;
}
