#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/capture.h"
#include "../include/concurrent_q.h"
#include "../include/gatekeeper.h"
#include "../scheduler/scheduler.h"  

int main(int argc, char *argv[]) {
    /* Parse command line arguments:
       argv[1]: CSV file path (default: ../packets.csv)
       argv[2]: delay in milliseconds (default: 10)
       argv[3]: source IP in hex (default: 0x7F000001 = 127.0.0.1)
    */
    const char *csv_file = "../packets.csv";
    uint32_t delay_ms = 10;
    uint32_t src_ip = 0x7F000001;  /* 127.0.0.1 */
    
    if (argc > 1) csv_file = argv[1];
    if (argc > 2) delay_ms = atoi(argv[2]);
    if (argc > 3) sscanf(argv[3], "%x", &src_ip);

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║      ArborFlow - CSV Packet Engine     ║\n");
    printf("║     Reading from: %s  ║\n", csv_file);
    printf("╚════════════════════════════════════════╝\n\n");

    /* INIT QUEUE */
    printf("1. Creating concurrent queue (capacity: 1024)\n");
    ConcurrentQueue *queue = cq_create();

    /* INIT CAPTURE (CSV mode) */
    printf("2. Initializing CSV packet reader\n");
    CaptureEngine ce;
    if (capture_init(&ce, csv_file, queue, delay_ms, src_ip) != 0) {
        printf("❌ Capture init failed\n");
        return 1;
    }

    /* INIT GATEKEEPER */
    printf("3. Initializing firewall (Gatekeeper)\n");
    Gatekeeper gk;
    if (gk_init(&gk) != 0) {
        printf("❌ Gatekeeper init failed\n");
        return 1;
    }

    /* INIT SCHEDULER */
    printf("4. Initializing packet scheduler (priority heap)\n");
    Scheduler scheduler;
    scheduler_init(&scheduler);

    /* START CAPTURE */
    printf("5. Starting packet reading thread\n");
    capture_start(&ce);

    printf("\n🚀 ArborFlow Running...\n");
    printf("   Processing packets with %u ms delay\n", delay_ms);
    printf("   Source IP: %u.%u.%u.%u\n",
           (src_ip >> 24) & 0xFF,
           (src_ip >> 16) & 0xFF,
           (src_ip >> 8) & 0xFF,
           src_ip & 0xFF);
    printf("\n");

    Packet pkt;
    Packet out_pkt;
    uint32_t packets_processed = 0;
    uint32_t packets_blocked = 0;

    /* Main processing loop */
    while (1) {
        /* STEP 1: GET PACKET from queue */
        if (cq_dequeue(queue, &pkt)) {
            packets_processed++;

            /* STEP 2: FILTER - check if source IP is blacklisted */
            int decision = check_ip(&gk, pkt.src_ip);

            if (decision == 1) {
                /* Source IP is in blacklist - DROP packet */
                packets_blocked++;
                printf("[DROP] Packet %u blocked\n", packets_processed);
                continue;
            }

            /* STEP 3: SCHEDULER INSERT - add to priority heap */
            scheduler_insert(&scheduler, pkt);
        }

        /* STEP 4: PROCESS FROM HEAP - extract and display highest priority */
        if (!scheduler_empty(&scheduler)) {
            scheduler_extract(&scheduler, &out_pkt);

            printf("[PROCESS %u] ", packets_processed);
            printf("%u.%u.%u.%u -> %u.%u.%u.%u ",
                (out_pkt.src_ip >> 24) & 0xFF,
                (out_pkt.src_ip >> 16) & 0xFF,
                (out_pkt.src_ip >> 8) & 0xFF,
                out_pkt.src_ip & 0xFF,
                (out_pkt.dest_ip >> 24) & 0xFF,
                (out_pkt.dest_ip >> 16) & 0xFF,
                (out_pkt.dest_ip >> 8) & 0xFF,
                out_pkt.dest_ip & 0xFF);

            printf("| Proto:%d Size:%d Priority:%d\n",
                out_pkt.protocol,
                out_pkt.size,
                out_pkt.priority);
        }

        usleep(1000);
    }

    /* Cleanup (unreachable, but good practice) */
    capture_stop(&ce);
    gk_destroy(&gk);
    cq_destroy(queue);

    return 0;
}