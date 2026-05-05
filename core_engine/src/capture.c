/*
 * CSV-Based Packet Capture Engine
 * 
 * Reads pregenerated packets from CSV file and enqueues them
 * to a concurrent queue for firewall filtering and scheduling.
 */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "../include/capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

/* ============================================================================
 * Helper Functions for CSV Reading
 * ============================================================================ */

/*
 * parse_ip — Convert IP string (e.g., "192.168.1.1") to uint32_t
 * 
 * Example: "192.168.1.1" → 0xC0A80101
 * Uses bit-shifting to combine 4 octets into 32-bit integer
 */
static uint32_t parse_ip(const char *ip_str) {
    uint32_t ip = 0;
    int octets[4];
    
    /* Parse 4 decimal numbers separated by dots */
    if (sscanf(ip_str, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]) == 4) {
        /* Combine octets: octet0.octet1.octet2.octet3 */
        ip = (uint32_t)octets[0] << 24 |  /* First octet: bits 24-31 */
             (uint32_t)octets[1] << 16 |  /* Second octet: bits 16-23 */
             (uint32_t)octets[2] << 8  |  /* Third octet: bits 8-15 */
             (uint32_t)octets[3];         /* Fourth octet: bits 0-7 */
    }
    return ip;
}

/*
 * delay_ms — Sleep for specified milliseconds
 * 
 * Uses nanosleep() for accurate millisecond delays
 * Example: delay_ms(10) sleeps 10ms
 */
static void delay_ms(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;           /* Convert to seconds */
    ts.tv_nsec = (ms % 1000) * 1000000;  /* Convert remaining ms to nanoseconds */
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * CSV Reader Thread Function
 * ============================================================================ */

/*
 * csv_capture_thread_func — Read packets from CSV file and enqueue them
 * 
 * This function runs in a background thread and:
 * 1. Opens the CSV file (packets.csv)
 * 2. Skips the header line
 * 3. Reads each CSV line (one packet per line)
 * 4. Parses the CSV format: dest_ip,protocol,port,size,priority
 * 5. Creates a Packet struct with fixed src_ip and CSV dest_ip
 * 6. Enqueues to concurrent queue
 * 7. Applies delay_ms between packets
 * 8. Continues until all packets read or stopped
 * 
 * Thread stops when: ce->running = 0
 */
void *csv_capture_thread_func(void *arg) {
    CaptureEngine *ce = (CaptureEngine *)arg;
    
    /* Step 1: Open CSV file for reading */
    FILE *f = fopen(ce->csv_file, "r");
    if (!f) {
        fprintf(stderr, "[Capture] Failed to open %s: %s\n", ce->csv_file, strerror(errno));
        return NULL;
    }
    
    printf("[Capture] Opened CSV file: %s\n", ce->csv_file);
    
    char line[512];  /* Buffer for each CSV line */
    uint32_t packet_count = 0;
    
    /* Step 2: Skip header line (dest_ip,protocol,port,size,priority) */
    if (fgets(line, sizeof(line), f) == NULL) {
        fprintf(stderr, "[Capture] CSV file is empty\n");
        fclose(f);
        return NULL;
    }
    
    printf("[Capture] Skipped CSV header\n");
    
    /* Step 3: Read CSV lines while thread is running */
    while (ce->running && fgets(line, sizeof(line), f) != NULL) {
        
        /* Step 4: Parse CSV line */
        char dest_ip_str[20];  /* Buffer for IP address string */
        int protocol, port, size, priority;
        
        /* Parse CSV format: dest_ip,protocol,port,size,priority */
        /* %19[^,] means: read up to 19 chars until comma (don't read the comma) */
        if (sscanf(line, "%19[^,],%d,%d,%d,%d", 
                   dest_ip_str, &protocol, &port, &size, &priority) != 5) {
            /* If parse fails, skip this line */
            continue;
        }
        
        /* Convert IP string to uint32_t binary format */
        uint32_t dest_ip = parse_ip(dest_ip_str);
        
        /* Step 5: Create Packet struct */
        Packet pkt = {
            .src_ip = ce->src_ip,      /* Fixed source IP (same for all) */
            .dest_ip = dest_ip,        /* Variable destination IP (from CSV) */
            .protocol = protocol,      /* 6=TCP or 17=UDP */
            .size = size,              /* Packet size in bytes */
            .priority = priority       /* 9=DNS, 8=HTTP/HTTPS, 5=default */
        };
        
        /* Step 6: Try to enqueue packet to concurrent queue */
        if (!cq_enqueue(ce->queue, pkt)) {
            /* Queue is full - drop packet and continue */
            fprintf(stderr, "[Capture] Queue full, dropped packet\n");
            continue;
        }
        
        packet_count++;
        
        /* Step 7: Apply delay between packets (if configured) */
        if (ce->delay_ms > 0) {
            delay_ms(ce->delay_ms);
        }
    }
    
    /* Finished reading all packets from CSV */
    printf("[Capture] CSV processing complete: %u packets read\n", packet_count);
    fclose(f);
    
    return NULL;
}

/* ============================================================================
 * Public API Functions
 * ============================================================================ */

/*
 * capture_init — Initialize capture engine for CSV packet reading
 * 
 * Validates inputs, checks if CSV file exists, and sets up CaptureEngine struct
 */
int capture_init(CaptureEngine *ce, const char *csv_file, ConcurrentQueue *queue,
                 uint32_t delay_ms, uint32_t src_ip) {
    
    /* Validate input parameters */
    if (!ce || !csv_file || !queue) {
        fprintf(stderr, "[Capture] Invalid parameters\n");
        return -1;
    }
    
    /* Check if CSV file exists and is readable */
    FILE *f = fopen(csv_file, "r");
    if (!f) {
        fprintf(stderr, "[Capture] Cannot open %s: %s\n", csv_file, strerror(errno));
        return -1;
    }
    fclose(f);
    
    /* Initialize CaptureEngine struct */
    memset(ce, 0, sizeof(CaptureEngine));
    ce->csv_file = strdup(csv_file);  /* Allocate and copy filename */
    ce->queue = queue;                 /* Reference to queue */
    ce->delay_ms = delay_ms;           /* Delay between packets */
    ce->src_ip = src_ip;               /* Fixed source IP for all packets */
    ce->running = 0;                   /* Not yet running */
    
    printf("[Capture] Initialized CSV mode: file=%s, delay=%u ms, src_ip=0x%x\n",
           csv_file, delay_ms, src_ip);
    
    return 0;
}

/*
 * capture_start — Start reading packets in background thread
 * 
 * Creates a new thread to run csv_capture_thread_func
 * Main program continues immediately (non-blocking)
 */
int capture_start(CaptureEngine *ce) {
    
    /* Validate parameters */
    if (!ce || ce->running) {
        fprintf(stderr, "[Capture] Already running or invalid\n");
        return -1;
    }
    
    /* Mark as running before creating thread */
    ce->running = 1;
    
    /* Create background thread for CSV reading */
    if (pthread_create(&ce->capture_thread, NULL, csv_capture_thread_func, ce) != 0) {
        fprintf(stderr, "[Capture] Failed to create thread\n");
        ce->running = 0;
        return -1;
    }
    
    printf("[Capture] Started CSV reading thread\n");
    return 0;
}

/*
 * capture_stop — Stop reading and cleanup resources
 * 
 * Signals thread to stop, waits for it to finish, and frees memory
 */
void capture_stop(CaptureEngine *ce) {
    
    if (!ce) return;
    
    /* Signal thread to stop */
    ce->running = 0;
    
    /* Wait for thread to finish (join) */
    pthread_join(ce->capture_thread, NULL);
    
    /* Free allocated memory */
    if (ce->csv_file) {
        free(ce->csv_file);
        ce->csv_file = NULL;
    }
    
    printf("[Capture] CSV reading stopped\n");
}

}