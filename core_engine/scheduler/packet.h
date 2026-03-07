#ifndef PACKET_H
#define PACKET_H
#include <stdint.h>
typedef struct{
    uint32_t src_ip;
    uint32_t dest_ip;
    int protocol;
    int size;
    int priority;
}Packet;
#endif