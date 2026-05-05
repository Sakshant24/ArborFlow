#ifndef PACKET_H
#define PACKET_H
#include <stdint.h>
typedef struct{
    uint32_t src_ip;
    uint32_t dest_ip;
    uint16_t src_port;
    uint16_t dst_port;
    int protocol;
    int size;
    int priority;
}Packet;
#endif