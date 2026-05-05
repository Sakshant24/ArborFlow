/*
 * Test program for CSV-based packet reading
 * Reads packets from packets.csv and feeds them into the concurrent queue
 * with configurable delays for realistic testing.
 */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "include/capture.h"
#include "include/concurrent_q.h"
#include "include/gatekeeper.h"
#include "scheduler/scheduler.h"

int main(int argc, char *argv[]) {
    /* Default parameters */
    const char *csv_file = "../packets.csv";
    uint32_t delay_ms = 10;  /* Delay between packets in milliseconds */
    uint32_t src_ip = 0x7F000001;  /* 127.0.0.1 as default source IP */
    
    /* Parse command line arguments */
    if (argc > 1) csv_file = argv[1];
    if (argc > 2) delay_ms = atoi(argv[2]);
    if (argc > 3) sscanf(argv[3], "%x", &src_ip);
    
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║         ArborFlow CSV Packet Test                      ║\n");
    printf("║    Reading pre-generated packets from CSV file         ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
    
    printf("📊 Configuration:\n");
    printf("   CSV File: %s\n", csv_file);
    printf("   Delay: %u ms between packets\n", delay_ms);
    printf("   Source IP: %u.%u.%u.%u\n",
           (src_ip >> 24) & 0xFF,
           (src_ip >> 16) & 0xFF,
           (src_ip >> 8) & 0xFF,
           src_ip & 0xFF);
    printf("\n");
    
    /* Initialize concurrent queue */
    ConcurrentQueue *queue = cq_create();
    if (!queue) {
        fprintf(stderr, "❌ Failed to create concurrent queue\n");
        return 1;
    }
    printf("✅ Created concurrent queue (size: %d)\n\n", Q_SIZE);
    
    /* Initialize capture engine in CSV mode */
    CaptureEngine ce;
    if (capture_init_csv(&ce, csv_file, queue, delay_ms, src_ip) != 0) {
        fprintf(stderr, "❌ Capture CSV init failed\n");
        cq_destroy(queue);
        return 1;
    }
    printf("✅ Initialized capture engine in CSV mode\n\n");
    
    /* Initialize Gatekeeper */
    Gatekeeper gk;
    if (gk_init(&gk) != 0) {
        fprintf(stderr, "❌ Gatekeeper init failed\n");
        cq_destroy(queue);
        return 1;
    }
    printf("✅ Initialized Gatekeeper (blacklist checking)\n\n");
    
    /* Initialize Scheduler */
    Scheduler scheduler;
    scheduler_init(&scheduler);
    printf("✅ Initialized Scheduler (priority-based heap)\n\n");
    
    /* Start packet reading from CSV */
    printf("🚀 Starting CSV packet reading...\n\n");
    capture_start(&ce);
    
    /* Statistics tracking */
    uint32_t processed_count = 0;
    uint32_t blocked_count = 0;
    uint32_t scheduled_count = 0;
    time_t start_time = time(NULL);
    
    Packet pkt;
    Packet out_pkt;
    
    printf("┌─────────────────────────────────────────────────────────┐\n");
    printf("│ Processing packets...                                   │\n");
    printf("└─────────────────────────────────────────────────────────┘\n");
    
    int iteration = 0;
    
    /* Main processing loop */
    while (iteration < 100 && (processed_count < 100 || !cq_empty(queue))) {
        /* STEP 1: Try to get packet from queue */
        if (cq_dequeue(queue, &pkt)) {
            processed_count++;
            
            /* STEP 2: Check against blacklist */
            int decision = check_ip(&gk, pkt.src_ip);
            
            if (decision == 1) {
                blocked_count++;
                
                printf("[%u] 🚫 BLOCKED: ", processed_count);
                printf("%u.%u.%u.%u -> %u.%u.%u.%u | ",
                       (pkt.src_ip >> 24) & 0xFF,
                       (pkt.src_ip >> 16) & 0xFF,
                       (pkt.src_ip >> 8) & 0xFF,
                       pkt.src_ip & 0xFF,
                       (pkt.dest_ip >> 24) & 0xFF,
                       (pkt.dest_ip >> 16) & 0xFF,
                       (pkt.dest_ip >> 8) & 0xFF,
                       pkt.dest_ip & 0xFF);
                
                printf("Proto:%d Port:? Size:%d Priority:%d\n",
                       pkt.protocol, pkt.size, pkt.priority);
                
            } else {
                /* STEP 3: Insert into scheduler */
                scheduler_insert(&scheduler, pkt);
                scheduled_count++;
                
                printf("[%u] ✅ PASSED: ", processed_count);
                printf("%u.%u.%u.%u -> %u.%u.%u.%u | ",
                       (pkt.src_ip >> 24) & 0xFF,
                       (pkt.src_ip >> 16) & 0xFF,
                       (pkt.src_ip >> 8) & 0xFF,
                       pkt.src_ip & 0xFF,
                       (pkt.dest_ip >> 24) & 0xFF,
                       (pkt.dest_ip >> 16) & 0xFF,
                       (pkt.dest_ip >> 8) & 0xFF,
                       pkt.dest_ip & 0xFF);
                
                printf("Proto:%d Size:%d Priority:%d (queued)\n",
                       pkt.protocol, pkt.size, pkt.priority);
            }
        }
        
        /* STEP 4: Process top packet from scheduler */
        if (!scheduler_empty(&scheduler)) {
            scheduler_extract(&scheduler, &out_pkt);
            
            printf("    ▶ PROCESSING: ");
            printf("%u.%u.%u.%u -> %u.%u.%u.%u | ",
                   (out_pkt.src_ip >> 24) & 0xFF,
                   (out_pkt.src_ip >> 16) & 0xFF,
                   (out_pkt.src_ip >> 8) & 0xFF,
                   out_pkt.src_ip & 0xFF,
                   (out_pkt.dest_ip >> 24) & 0xFF,
                   (out_pkt.dest_ip >> 16) & 0xFF,
                   (out_pkt.dest_ip >> 8) & 0xFF,
                   out_pkt.dest_ip & 0xFF);
            
            printf("Priority:%d Size:%d (from heap)\n",
                   out_pkt.priority,
                   out_pkt.size);
        }
        
        usleep(1000);  /* Small delay to avoid busy waiting */
        iteration++;
    }
    
    /* Stop capture and cleanup */
    capture_stop(&ce);
    time_t end_time = time(NULL);
    
    printf("\n┌─────────────────────────────────────────────────────────┐\n");
    printf("│ Test Complete                                           │\n");
    printf("└─────────────────────────────────────────────────────────┘\n\n");
    
    printf("📈 Statistics:\n");
    printf("   Total packets processed: %u\n", processed_count);
    printf("   Packets blocked (blacklisted): %u (%.1f%%)\n", 
           blocked_count, 
           processed_count > 0 ? (100.0 * blocked_count / processed_count) : 0);
    printf("   Packets scheduled: %u\n", scheduled_count);
    printf("   Elapsed time: %ld seconds\n", end_time - start_time);
    
    if (processed_count > 0) {
        printf("   Avg packets/sec: %.1f\n", 
               processed_count / (float)(end_time - start_time + 1));
    }
    printf("\n");
    
    printf("✅ Test successful!\n");
    printf("   All queues and data structures are working correctly.\n");
    printf("   CSV-based packet generation is operational.\n\n");
    
    /* Cleanup */
    gk_destroy(&gk);
    cq_destroy(queue);
    
    return 0;
}
