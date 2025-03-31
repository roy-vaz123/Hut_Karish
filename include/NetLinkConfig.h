#ifndef NETLINKCONFIG_H
#define NETLINKCONFIG_H


#ifdef __KERNEL__

#include <linux/types.h>   // for __u32, __u16
#include <linux/list.h>    // for list_head
typedef __u32 uint32_t;
typedef __u16 uint16_t;

#else

#include <stdint.h>        // only for user-space
// Dummy list_head just for matching layout (never used)
struct list_head {
    void* next;
    void* prev;
};

#endif

// Packet info structure
struct pckt_info {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t payload_size;
    char proto; // 'T' or 'U'
    struct list_head list;
};


// Protocol flags
#define PROTO_TCP 'T'
#define PROTO_UDP 'U'


#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 // maximum payload size

#endif // NETLINK_CONFIG_H
