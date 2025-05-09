#ifndef NETLINKCONFIG_H
#define NETLINKCONFIG_H


#ifdef __KERNEL__
// for kernel space only
#include <linux/types.h>   // for __u32, __u16
typedef __u32 uint32_t;
typedef __u16 uint16_t;
#else

// only for user-space
#include <stdint.h> 
#include <sys/types.h>       

#endif

// Packet info structure
// IPs are in network byte order
// Ports are in host byte order
struct pckt_info {
    // network byte order
    uint32_t src_ip;
    uint32_t dst_ip;
    // host byte order
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
