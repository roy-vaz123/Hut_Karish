// Basic kernel module
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
// Netfilter hook
#include <linux/netfilter.h> // Core netfilter definitions
#include <linux/netfilter_ipv4.h>// Hook specificaly to ipv4 packets, since its the most common 
// Parse information from frame to get src dest ip port
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>


// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roy");
MODULE_DESCRIPTION("Basic Packet Sniffer Kernel Module");
MODULE_VERSION("0.1");


static struct nf_hook_ops nfho;  // Netfilter hook options struct

// Retrive ip header from a packet in the socket buffer and printk the packet cought
static unsigned int packet_sniffer_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){
    
    struct iphdr *ip_header;

    if (!skb)// Packets comes as sk_buff struct, if skb is NULL, it means no packet was captured, or a bug
        return NF_ACCEPT;

    ip_header = ip_hdr(skb);
    if (!ip_header)// If failed to retrive ip header from packet (non ip packet or corrupted) using ip_hdr() macro
        return NF_ACCEPT;
    
    u8 protocol = ip_header->protocol;// Extract the protocol from the ip header
    
    // Usigned integers to store the source and destination ports after conversion from network byte order to host byte order 
    u16 src_port = 0;
    u16 dst_port = 0;
            
    // Check the protocol and handle accordingly
    switch (protocol) {
        
        case IPPROTO_TCP:// handle TCP    
            struct tcphdr *tcp_header;
            tcp_header = tcp_hdr(skb);// Extract the TCP header from the packet using macro
            
            if (!tcp_header)
                return NF_ACCEPT;
            
            // Extract source and destination ports from the TCP header, convert them to host byte order (u16) from network byte order
            src_port = ntohs(tcp_header->source);
            dst_port = ntohs(tcp_header->dest);
            
            printk(KERN_INFO "[sniffer] Caught a TCP packet: ");
            break;
        
        
        case IPPROTO_UDP:// handle UDP, same as tcp different macro
            struct udphdr *udp_header;
            
            udp_header = udp_hdr(skb);// Extract the UDP header from the packet using macro
            if (!udp_header)
                return NF_ACCEPT;
            
            src_port = ntohs(udp_header->source);
            dst_port = ntohs(udp_header->dest);

            printk(KERN_INFO "[sniffer] Caught a UDP packet: ");
            break;
        
        default:
            // for other protocols do nothing for now
            printk(KERN_INFO "[sniffer] Caught a packet (unknown protocol and ports): ");
            break;
    }


    // Print packets source and dest adresses (from ip header)
    printk("src=%pI4:%d dst=%pI4:%d\n", &ip_header->saddr, src_port, &ip_header->daddr, dst_port);

    return NF_ACCEPT;  // Let the packet continue normally
}


// init function (runs on module load)
static int __init sniffer_init(void) {
    printk(KERN_INFO "[sniffer] Module loaded.\n");

    nfho.hook = packet_sniffer_hook;       // The function to run when a packet hits the hook
    nfho.hooknum = NF_INET_PRE_ROUTING;    // Places the hook right after the packet enters the system, wait for ipv4 packets
    nfho.pf = PF_INET;                     // IPv4 packets only
    nfho.priority = NF_IP_PRI_FIRST;       // Run before other hooks (highest priority)

    nf_register_net_hook(&init_net, &nfho); // Register the hook with netfilter (init_net refers to the default network namespace)

    return 0;
}

// exit function (runs on module unload)
static void __exit sniffer_exit(void) {
    nf_unregister_net_hook(&init_net, &nfho); // Unregister the hook (if the hook is not unregistered, it will remain active even after the module is unloaded) 
    printk(KERN_INFO "[sniffer] Module unloaded.\n");
}

// Register the init and exit functions
module_init(sniffer_init);
module_exit(sniffer_exit);
