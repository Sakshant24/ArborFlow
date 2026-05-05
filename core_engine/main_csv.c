#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "include/gatekeeper.h"
#include "scheduler/scheduler.h"

#define PACKETS_FILE "../data/packets.csv"
#define BLACKLIST_FILE "../data/blacklist.csv"

typedef struct {
    char dest_ip[16];
    int protocol;
    int port;
    int size;
    int priority;
} CSVRow;

int load_packets(const char *filename, CSVRow **packets, int *count) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("[Error] Cannot open %s\n", filename);
        return -1;
    }

    char line[256];
    int capacity = 10000;
    *packets = (CSVRow *)malloc(capacity * sizeof(CSVRow));
    *count = 0;

    fgets(line, sizeof(line), fp); // Skip header

    while (fgets(line, sizeof(line), fp)) {
        if (*count >= capacity) {
            capacity *= 2;
            *packets = (CSVRow *)realloc(*packets, capacity * sizeof(CSVRow));
        }

        CSVRow *p = &(*packets)[*count];
        sscanf(line, "%15[^,],%d,%d,%d,%d", 
               p->dest_ip, &p->protocol, &p->port, &p->size, &p->priority);
        (*count)++;
    }

    fclose(fp);
    return 0;
}

uint32_t ip_to_uint32(const char *ip_str) {
    unsigned int a, b, c, d;
    sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    return ((a << 24) | (b << 16) | (c << 8) | d);
}

const char* get_protocol_name(int proto) {
    switch(proto) {
        case 6: return "TCP";
        case 17: return "UDP";
        case 1: return "ICMP";
        default: return "UNK";
    }
}

int main(int argc, char *argv[]) {
    printf("============================================================\n");
    printf("   ArborFlow - C Network Processing Engine (Dataset Mode)\n");
    printf("============================================================\n\n");

    CSVRow *packets = NULL;
    int packet_count = 0;

    printf("[1] Loading packets from dataset...\n");
    if (load_packets(PACKETS_FILE, &packets, &packet_count) != 0) {
        return 1;
    }
    printf("[SUCCESS] Loaded %d packets from %s\n\n", packet_count, PACKETS_FILE);

    printf("[2] Initializing Gatekeeper...\n");
    Gatekeeper gk;
    if (gk_init(&gk) != 0) {
        printf("Gatekeeper init failed\n");
        return 1;
    }

    printf("[3] Loading blacklist...\n");
    int loaded = gk_load_blacklist(&gk, BLACKLIST_FILE);
    if (loaded < 0) {
        printf("Warning: Failed to load blacklist\n");
    }
    printf("[SUCCESS] Loaded %d blacklisted IPs\n\n", loaded);

    printf("[4] Initializing Scheduler...\n");
    Scheduler scheduler;
    scheduler_init(&scheduler);

    printf("\n============================================================\n");
    printf("   🚀 ArborFlow Running... (Press Ctrl+C to stop)\n");
    printf("============================================================\n\n");

    int idx = 0;
    int dropped = 0, passed = 0;

    while (1) {
        CSVRow *row = &packets[idx];
        idx = (idx + 1) % packet_count;

        uint32_t ip = ip_to_uint32(row->dest_ip);

        int decision = check_ip(&gk, ip);

        if (decision == 1) {
            printf("[DROP] %s blocked by Gatekeeper\n", row->dest_ip);
            dropped++;
        } else {
            Packet pkt = {
                .dest_ip = ip,
                .src_ip = ((rand() % 192 + 1) << 24) | 
                          ((rand() % 168 + 1) << 16) | 
                          ((rand() % 254 + 1) << 8) | 
                          (rand() % 254 + 1),
                .src_port = (uint16_t)(rand() % 65535),
                .dst_port = (uint16_t)row->port,
                .protocol = row->protocol,
                .size = row->size,
                .priority = row->priority
            };

            scheduler_insert(&scheduler, pkt);
            passed++;

            Packet out_pkt;
            if (!scheduler_empty(&scheduler)) {
                scheduler_extract(&scheduler, &out_pkt);

                printf("[PROCESS] ");
                printf("%u.%u.%u.%u -> %u.%u.%u.%u ",
                    (out_pkt.src_ip >> 24) & 0xFF,
                    (out_pkt.src_ip >> 16) & 0xFF,
                    (out_pkt.src_ip >> 8) & 0xFF,
                    out_pkt.src_ip & 0xFF,
                    (out_pkt.dest_ip >> 24) & 0xFF,
                    (out_pkt.dest_ip >> 16) & 0xFF,
                    (out_pkt.dest_ip >> 8) & 0xFF,
                    out_pkt.dest_ip & 0xFF);
                
                printf("Proto:%s Port:%d Size:%d Priority:%d\n",
                    get_protocol_name(out_pkt.protocol),
                    out_pkt.dst_port,
                    out_pkt.size,
                    out_pkt.priority);
            }
        }

        usleep(500000); // 0.5 second delay
    }

    printf("\n[STATS] Dropped: %d, Passed: %d\n", dropped, passed);

    gk_destroy(&gk);
    free(packets);

    return 0;
}