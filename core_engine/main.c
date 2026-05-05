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
    if (argc < 2) {
        printf("Usage: %s <network_device>\n", argv[0]);
        printf("Available devices:\n");
        capture_list_devices();
        return 1;
    }

    const char *device = argv[1];

    /* INIT QUEUE */
    ConcurrentQueue *queue = cq_create();

    /* INIT CAPTURE */
    CaptureEngine ce;
    if (capture_init(&ce, device, queue) != 0) {
        printf("Capture init failed\n");
        return 1;
    }

    /* INIT GATEKEEPER */
    Gatekeeper gk;
    if (gk_init(&gk) != 0) {
        printf("Gatekeeper init failed\n");
        return 1;
    }

    /* INIT SCHEDULER */
    Scheduler scheduler;
    scheduler_init(&scheduler);

    /* START CAPTURE */
    capture_start(&ce);

    printf("\n🚀 ArborFlow Running...\n\n");

    Packet pkt;
    Packet out_pkt;

    while (1) {
        /* STEP 1: GET PACKET */
        if (cq_dequeue(queue, &pkt)) {

            /* STEP 2: FILTER */
            int decision = check_ip(&gk, pkt.src_ip);

            if (decision == 1) {
                printf("[DROP] Packet blocked\n");
                continue;
            }

            /* STEP 3: SCHEDULER INSERT */
            scheduler_insert(&scheduler, pkt);
        }

        /* STEP 4: PROCESS FROM HEAP */
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

            printf("Priority:%d Size:%d\n",
                out_pkt.priority,
                out_pkt.size);
        }

        usleep(1000);
    }

    capture_stop(&ce);
    gk_destroy(&gk);
    cq_destroy(queue);

    return 0;
}