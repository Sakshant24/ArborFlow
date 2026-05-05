#ifndef CAPTURE_H
#define CAPTURE_H

/*
 *
 Uses either libpcap for live packet sniffing OR reads pregenerated packets from CSV.
 Modes:
 1. Live capture (libpcap): Sniffs packets from network interface
    - Parses Ethernet/IP/TCP/UDP headers to extract packet fields
 2. CSV mode: Reads pregenerated packets from CSV file with delays
    - CSV format: dest_ip,protocol,port,size,priority
    - Packets enqueued with configurable delay between packets
 
 In both modes, packets are:
 - Processed and enqueued to concurrent queue for processing threads
 - Run in separate thread to avoid blocking main processing
 */

#include <pcap.h>
#include <pthread.h>
#include <stdio.h>
#include "../scheduler/packet.h"
#include "../include/concurrent_q.h"

#define CAPTURE_SNAPLEN  65535  /* Maximum bytes to capture per packet, 64KB here for all data */
#define CAPTURE_PROMISC  1      /* Promiscuous mode, read all data */
#define CAPTURE_TIMEOUT  1000   /* Read timeout in ms, 1 ms for now */

/* Capture mode enumeration */
typedef enum {
    CAPTURE_MODE_LIVE,   /* Use libpcap for live packet capture */
    CAPTURE_MODE_CSV     /* Read packets from CSV file */
} CaptureMode;

/* Capture engine state */
typedef struct {
    CaptureMode     mode;           /* Capture mode (LIVE or CSV) */
    
    /* For LIVE mode */
    pcap_t          *handle;        /* libpcap handle */
    char            *device;        /* Network interface name */
    
    /* For CSV mode */
    char            *csv_file;      /* Path to CSV file */
    uint32_t        delay_ms;       /* Delay in milliseconds between packets */
    uint32_t        src_ip;         /* Fixed source IP for CSV packets */
    
    /* Common */
    ConcurrentQueue *queue;         /* Queue to push captured packets */
    pthread_t       capture_thread; /* Thread for packet capture, This is the thread ID */
    int             running;        /* Flag to stop capture */
} CaptureEngine;

/*
 capture_init — Initialize capture engine with network device (LIVE mode).
 Returns 0 on success, -1 on failure.
 */
int capture_init(CaptureEngine *ce, const char *device, ConcurrentQueue *queue);

/*
 capture_init_csv — Initialize capture engine for CSV mode.
 Parameters:
 - ce: CaptureEngine struct to initialize
 - csv_file: Path to packets.csv
 - queue: ConcurrentQueue for enqueuing packets
 - delay_ms: Delay in milliseconds between packets (for simulation)
 - src_ip: Fixed source IP for all packets from CSV (IP address as uint32_t)
 
 Returns 0 on success, -1 on failure.
 */
int capture_init_csv(CaptureEngine *ce, const char *csv_file, ConcurrentQueue *queue, 
                     uint32_t delay_ms, uint32_t src_ip);

/*
 capture_start — Start packet capture in background thread.
 Returns 0 on success, -1 on failure.
 */
int capture_start(CaptureEngine *ce);

/*
 capture_stop — Stop capture and cleanup.
 */
void capture_stop(CaptureEngine *ce);

/*
 capture_list_devices — List available network devices.
 */
void capture_list_devices(void);

#endif