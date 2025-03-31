#ifndef NETLINKCONFIG_H
#define NETLINKCONFIG_H


#ifdef __KERNEL__

#include <linux/types.h>   // for __u32, __u16
typedef __u32 uint32_t;
typedef __u16 uint16_t;

#else
#include <stdint.h>        // only for user-space

#endif

// Packet info structure
struct pckt_info {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t payload_size;
    char proto; // 'T' or 'U'
};


// Protocol flags
#define PROTO_TCP 'T'
#define PROTO_UDP 'U'


#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 // maximum payload size

#endif // NETLINK_CONFIG_H
