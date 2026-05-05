#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/capture.h"
#include "../include/concurrent_q.h"
#include "../include/gatekeeper.h"

/*
 * capture_demo.c — Demo of Capture Engine Integration
 * ----------------------------------------------------
 * Demonstrates capturing packets, processing them through
 * the Gatekeeper, and scheduling.
 */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <network_device>\n", argv[0]);
        fprintf(stderr, "Available devices:\n");
        capture_list_devices();
        return 1;
    }

    const char *device = argv[1];

    /* Initialize components */
    ConcurrentQueue *queue = cq_create();
    if (!queue) {
        fprintf(stderr, "Failed to create queue\n");
        return 1;
    }

    CaptureEngine ce;
    if (capture_init(&ce, device, queue) != 0) {
        fprintf(stderr, "Failed to initialize capture engine\n");
        cq_destroy(queue);
        return 1;
    }

    Gatekeeper gk;
    if (gk_init(&gk) != 0) {
        fprintf(stderr, "Failed to initialize gatekeeper\n");
        capture_stop(&ce);
        cq_destroy(queue);
        return 1;
    }

    /* Start capture */
    if (capture_start(&ce) != 0) {
        fprintf(stderr, "Failed to start capture\n");
        gk_destroy(&gk);
        capture_stop(&ce);
        cq_destroy(queue);
        return 1;
    }

    printf("Capturing packets... Press Ctrl+C to stop\n");

    /* Main processing loop */
    Packet pkt;
    int packet_count = 0;

    while (1) {
        /* Dequeue packet from capture queue */
        if (cq_dequeue(queue, &pkt)) {
            packet_count++;

            /* Process through Gatekeeper */
            int decision = check_ip(&gk, pkt.src_ip);
            const char *action = decision ? "DROP" : "PASS";

            /* Print packet info */
            printf("[%d] ", packet_count);
            printf("%u.%u.%u.%u -> %u.%u.%u.%u ",
                   (pkt.src_ip >> 24) & 0xFF, (pkt.src_ip >> 16) & 0xFF,
                   (pkt.src_ip >> 8) & 0xFF, pkt.src_ip & 0xFF,
                   (pkt.dest_ip >> 24) & 0xFF, (pkt.dest_ip >> 16) & 0xFF,
                   (pkt.dest_ip >> 8) & 0xFF, pkt.dest_ip & 0xFF);
            printf("Proto:%d Size:%d Priority:%d -> %s\n",
                   pkt.protocol, pkt.size, pkt.priority, action);

            /* If passed, could enqueue to scheduler here */
            /* scheduler_insert(scheduler, pkt); */
        }

        /* Small delay to prevent busy waiting */
        usleep(1000);
    }

    /* Cleanup (won't reach here due to infinite loop) */
    capture_stop(&ce);
    gk_destroy(&gk);
    cq_destroy(queue);

    return 0;
}