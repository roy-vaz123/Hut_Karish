// basic kernel module headers
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
// netwoeking headers
#include <linux/netfilter.h> // Core netfilter definitions
#include <linux/netfilter_ipv4.h>// Hook specificaly to ipv4 packets, since its the most common 
#include <linux/ip.h>// Access ip header fields


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

    // Print packets source and dest adresses (from ip header)
    printk(KERN_INFO "[sniffer] Caught a packet: src=%pI4 dst=%pI4\n", &ip_header->saddr, &ip_header->daddr);

    return NF_ACCEPT;  // Let the packet continue normally
}

// init function (runs on module load)
static int __init sniffer_init(void) {
    printk(KERN_INFO "[sniffer] Module loaded.\n");

    nfho.hook = packet_sniffer_hook;       // The function to run when a packet hits the hook
    nfho.hooknum = NF_INET_PRE_ROUTING;    // Places the hook right after the packet enters the system
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
