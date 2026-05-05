#ifndef CAPTURE_H
#define CAPTURE_H

/*
 * CSV-Based Packet Reading Engine
 * 
 * Reads pregenerated packets from CSV file with configurable delays.
 * CSV format: dest_ip,protocol,port,size,priority
 * 
 * Packets are read in a background thread and enqueued to concurrent queue
 * for processing by firewall filtering and priority scheduling.
 */

#include <pthread.h>
#include <stdio.h>
#include "../scheduler/packet.h"
#include "../include/concurrent_q.h"

/* Capture engine state - for reading CSV packets */
typedef struct {
    char            *csv_file;      /* Path to CSV file (packets.csv) */
    uint32_t        delay_ms;       /* Delay in milliseconds between packets */
    uint32_t        src_ip;         /* Fixed source IP for all CSV packets */
    ConcurrentQueue *queue;         /* Queue to push captured packets */
    pthread_t       capture_thread; /* Thread for reading packets */
    int             running;        /* Flag to stop capture (1=running, 0=stop) */
} CaptureEngine;

/*
 * capture_init — Initialize capture engine for CSV packet reading.
 * 
 * Parameters:
 *   ce: CaptureEngine struct to initialize
 *   csv_file: Path to packets.csv file
 *   queue: ConcurrentQueue for enqueuing packets
 *   delay_ms: Delay in milliseconds between packets (0 = no delay)
 *   src_ip: Fixed source IP for all packets (as uint32_t)
 * 
 * Returns: 0 on success, -1 on failure
 */
int capture_init(CaptureEngine *ce, const char *csv_file, ConcurrentQueue *queue,
                 uint32_t delay_ms, uint32_t src_ip);

/*
 * capture_start — Start reading packets from CSV in background thread.
 * 
 * Returns: 0 on success, -1 on failure
 */
int capture_start(CaptureEngine *ce);

/*
 * capture_stop — Stop reading and cleanup resources.
 */
void capture_stop(CaptureEngine *ce);

#endif