// Headers that I copied from the internet
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

// Main headers
#include "../include/capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>

/* 
Packet callback function for libpcap
Libcap calls this function for each captured packet from pcap_loop.
We parse the packet and enqueue it to the concurrent queue for processing.
 */
void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet) {
    /*
    user is a pointer to the CaptureEngine instance
    header contains metadata about the packet (timestamp, length)
    packet is the raw packet data captured from the network interface   
    */

    // typecast user pointer to capture engine struct
    CaptureEngine *ce = (CaptureEngine *)user;

    /* Parse Ethernet header */
    struct ether_header *eth = (struct ether_header *)packet;

    /* Skip non-IP packets like ARP */
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return;  
    }

    /* Parse IP header */
    struct ip *ip_hdr = (struct ip *)(packet + sizeof(struct ether_header));

    uint32_t src_ip = ntohl(ip_hdr->ip_src.s_addr); // converts 32-bit IP address
    uint32_t dst_ip = ntohl(ip_hdr->ip_dst.s_addr);
    int protocol = ip_hdr->ip_p;
    int packet_size = header->len;  /* Total packet length */

    /* Determine priority
        - TCP packets to port 80/443 get higher priority (8)
        - UDP packets to port 53 get high priority (9)
        - Others get default priority (5)
    */
    int priority = 5;  /* Default */

    if (protocol == IPPROTO_TCP) {
        struct tcphdr *tcp_hdr = (struct tcphdr *)((u_char *)ip_hdr + (ip_hdr->ip_hl * 4));
        if (ntohs(tcp_hdr->th_dport) == 80 || ntohs(tcp_hdr->th_dport) == 443) {
            priority = 8;  /* HTTP/HTTPS higher priority */
        }
    } else if (protocol == IPPROTO_UDP) {
        struct udphdr *udp_hdr = (struct udphdr *)((u_char *)ip_hdr + (ip_hdr->ip_hl * 4));
        if (ntohs(udp_hdr->uh_dport) == 53) {
            priority = 9;  /* DNS high priority */
        }
    }

    /* Create Packet struct */
    Packet pkt = {
        .src_ip = src_ip,
        .dest_ip = dst_ip,
        .protocol = protocol,
        .size = packet_size,
        .priority = priority
    };

    /* Enqueue packet (non-blocking) */
    if (!cq_enqueue(ce->queue, pkt)) {
        /* Queue full - drop packet */
        fprintf(stderr, "[Capture] Queue full, dropping packet\n");
        return;
    }
}

/* Capture thread function */
void *capture_thread_func(void *arg) {
    CaptureEngine *ce = (CaptureEngine *)arg;

    /* Start packet capture loop, pcap_loop is inbuilt function of libcap */
    pcap_loop(ce->handle, -1, packet_handler, (u_char *)ce);

    return NULL;
}

int capture_init(CaptureEngine *ce, const char *device, ConcurrentQueue *queue) {
    // Check if any of the args are missing/NULL
    if (!ce || !device || !queue) return -1;

    // Allocate dynamic memory for capture engine struct and initialize fields
    memset(ce, 0, sizeof(CaptureEngine));
    ce->device = strdup(device);   // prevent dangling pointer, we will free this later in capture_stop
    ce->queue = queue;
    ce->running = 0;

    /* Open device for live capture */
    char errbuf[PCAP_ERRBUF_SIZE];   // error code buffer
    ce->handle = pcap_open_live(device, CAPTURE_SNAPLEN, CAPTURE_PROMISC,
                                CAPTURE_TIMEOUT, errbuf);
    if (!ce->handle) {
        fprintf(stderr, "[Capture] pcap_open_live failed: %s\n", errbuf);
        free(ce->device);
        return -1;
    }

    /* Compile and set filter (capture only IP packets) */
    struct bpf_program fp;
    char filter_exp[] = "ip";  /* Capture only IP packets */
    // pcap_compile takes human readable filter expression 
    if (pcap_compile(ce->handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "[Capture] pcap_compile failed: %s\n", pcap_geterr(ce->handle));
        pcap_close(ce->handle);
        free(ce->device);
        return -1;
    }
    // pcap applies the compiled filter to capture handle
    if (pcap_setfilter(ce->handle, &fp) == -1) {
        fprintf(stderr, "[Capture] pcap_setfilter failed: %s\n", pcap_geterr(ce->handle));
        pcap_freecode(&fp);
        pcap_close(ce->handle);
        free(ce->device);
        return -1;
    }
    pcap_freecode(&fp);

    return 0;
}

int capture_start(CaptureEngine *ce) {
    /* returns 0 on success and -1 on failure */ 

    // check if capture engine is already running or if ce is NULL
    if (!ce || ce->running) return -1;

    // if not running, mark it running
    ce->running = 1;

    /* Create capture thread */
    if (pthread_create(&ce->capture_thread, NULL, capture_thread_func, ce) != 0) {
        fprintf(stderr, "[Capture] Failed to create capture thread\n");
        ce->running = 0;
        return -1;
    }

    printf("[Capture] Started on device %s\n", ce->device);
    return 0; 
}

void capture_stop(CaptureEngine *ce) {
    if (!ce) return;

    ce->running = 0;
    pcap_breakloop(ce->handle);  /* Stop pcap_loop */

    /* Wait for thread to finish 
    Always join threads to avoid resource leaks and ensure clean shutdown.
    */
    pthread_join(ce->capture_thread, NULL);

    /* Cleanup */
    pcap_close(ce->handle);   // destroy pcap handle
    free(ce->device);   // frees the device string we allocated in capture_init

    printf("[Capture] Stopped\n");
}

void capture_list_devices(void) {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs;   // its a linked list of devices, we will iterate through it to print device names

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "[Capture] pcap_findalldevs failed: %s\n", errbuf);
        return;
    }

    printf("Available network devices:\n");
    for (pcap_if_t *d = alldevs; d; d = d->next) {
        printf("  %s", d->name);
        if (d->description) {
            printf(" (%s)", d->description);
        }
        printf("\n");
    }

    pcap_freealldevs(alldevs);
}