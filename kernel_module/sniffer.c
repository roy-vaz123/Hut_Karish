// Basic kernel module
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
// Netfilter hook
#include <linux/netfilter.h> // Core netfilter definitions
#include <linux/netfilter_ipv4.h>// Hook specificaly to ipv4 packets, since its the most common 
// skb headers (the packets come as sk_buff struct)
#include <linux/skbuff.h>
#include <linux/netdevice.h>
// Parse information from frame to get src dest ip port
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
// For saving data in the kernel space
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>  // needed for kmalloc and kfree
// Netlink socket
#include <net/sock.h>
#include <linux/netlink.h>

// Netlink constants
#define NETLINK_USER 31 // Netlink protocol number, 31 is in the range reserved for user-defined Netlink protocols
#define USER_MSG     (NETLINK_USER + 1) // ?
#define MAX_PAYLOAD  1024  // message size, max size of the payload in bytes


// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roy");
MODULE_DESCRIPTION("Basic Packet Sniffer Kernel Module");
MODULE_VERSION("0.1");


static struct nf_hook_ops nfho;  // Netfilter hook options struct
struct sock *nl_sk = NULL; // Netlink socket struct, used to send messages to user space


// Packet info struct and list
struct pckt_info {
    u32 src_ip;
    u32 dst_ip;
    u16 src_port;
    u16 dst_port;
    u32 payload_size;
    char proto; // 'T' for TCP, 'U' for UDP
    struct list_head list;
};


static LIST_HEAD(pckt_list);// List head to store packet info
static DEFINE_SPINLOCK(pckt_lock);// Spinlock to protect the list from concurrent access



// Send a message to user space process by pid using netlink
static void send_packet_list_to_user(u32 pid)
{
    struct pckt_info *entry, *tmp;
    struct pckt_info *packet_buf;
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    size_t count = 0, total_size, i = 0;

    // Create a Netlink message buffer, spinlock to protect the list from concurrent access to make sure 
    //the buffer is not modified while we are copying the packets
    spin_lock(&pckt_lock);
    // Count how many packets we have
    list_for_each_entry(entry, &pckt_list, list)
        count++;
    if (count == 0)
        return;  // No packets to send

    total_size = count * sizeof(struct pckt_info);// Calculate the total size of the packet buffer
    packet_buf = kmalloc(total_size, GFP_ATOMIC); // Allocate a temporary buffer to hold packet data
    if (!packet_buf)
        return;

    // Copy all packets into the buffer and clear the list
    list_for_each_entry_safe(entry, tmp, &pckt_list, list) {
        packet_buf[i++] = *entry;  
        list_del(&entry->list); // remove the entry from the list and free it    
        kfree(entry);              
    }
    spin_unlock(&pckt_lock);// The packet_buf is now filled with the packets info


    // Create a Netlink message buffer
    skb_out = nlmsg_new(total_size, GFP_KERNEL);
    if (!skb_out) {
        pr_err("sniffer: Failed to allocate Netlink skb\n");
        kfree(packet_buf);
        return;
    }
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, total_size, 0);
    if (!nlh) {
        pr_err("sniffer: nlmsg_put failed\n");
        kfree_skb(skb_out);
        kfree(packet_buf);
        return;
    }

    // Copy our packet buffer into the Netlink message
    memcpy(nlmsg_data(nlh), packet_buf, total_size);
    kfree(packet_buf);  // Free the kernel buffer after copying

    // Send the message to user space
    if (nlmsg_unicast(nl_sk, skb_out, pid) < 0) {
        pr_err("sniffer: Failed to send Netlink message\n");
    }
}


// Netlink Receive function (called when a message is received from user space)
static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    u32 pid;
    char *user_msg;

    // Extract the Netlink message header
    nlh = (struct nlmsghdr *)skb->data;

    // Get the sender PID (user-space process ID)
    pid = nlh->nlmsg_pid;

    // Extract the actual message data
    user_msg = (char *)nlmsg_data(nlh);

    pr_info("sniffer: received netlink message: %s\n", user_msg);

    // If the message is "get_packets", send the packet list to the user (to the sender proccess)
    if (strcmp(user_msg, "get_packets") == 0) {
        send_packet_list_to_user(pid);
    } else {
        pr_info("sniffer: Unknown command: %s\n", user_msg);
    }
}


// Retrive ip header from a packet in the socket buffer and pr_info the packet cought
static unsigned int packet_sniffer_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){
    
    // Frame headers structs
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    // Header lengths to calculate the size of the payload
    unsigned int payload_size;
    
    // Paceket info struct members
    u32 src_ip, dst_ip;
    u16 src_port, dst_port;
    char proto;
    struct pckt_info *entry; // The entry to be added to the list
    
    // Check if the skb is NULL or too short (packets comes as sk_buff struct, skb = the packet)
    if (!skb || skb->len < sizeof(struct iphdr)) {
        pr_info("[sniffer] Packet too short or skb is NULL. \n");
        return NF_ACCEPT;
    }
    
    iph = ip_hdr(skb);
    if (!iph)// If failed to retrive ip header from packet (non ip packet or corrupted) using ip_hdr() macro
        return NF_ACCEPT;
    
    // Get the source and destination IP addresses
    src_ip = ntohl(iph->saddr); // Convert to host byte order
    dst_ip = ntohl(iph->daddr);



    // Fill the packet info struct members according to the packet protocol (TCP or UDP)
    if (iph->protocol == IPPROTO_TCP) {// TCP
        tcph = tcp_hdr(skb);
        src_port = ntohs(tcph->source);
        dst_port = ntohs(tcph->dest);
        proto = 'T';
        payload_size = skb->len - skb_transport_offset(skb) - (tcph->doff * 4);  // skb_transport_offset(skb) to safely locate the start of TCP/UDP headers
    
    
    } else if (iph->protocol == IPPROTO_UDP) { // UDP
        udph = udp_hdr(skb);
        src_port = ntohs(udph->source);
        dst_port = ntohs(udph->dest);
        proto = 'U';
        payload_size = skb->len - skb_transport_offset(skb) - sizeof(struct udphdr); 
    
    
    } else { // Our module only supports TCP and UDP packets
        pr_info("[sniffer] Unsupported protocol: %d \n", iph->protocol);
        return NF_ACCEPT;
    }
                
    
    // pr_info the packet info, for debugging
    pr_info("[sniffer] Packet type %c Src IP: %pI4, Dst IP: %pI4, Src Port: %u, Dst Port: %u, Payload: %u bytes \n", proto, &src_ip, &dst_ip, src_port, dst_port, payload_size);
 
    entry = kmalloc(sizeof(*entry), GFP_ATOMIC);// Allocate memory for enrty in the list, GFP_ATOMIC prevents sleeping in the kernel space
    if (!entry)// If kmalloc failed
        return NF_ACCEPT;

    // Fill the entry with the packet info
    entry->src_ip = src_ip;
    entry->dst_ip = dst_ip;
    entry->src_port = src_port;
    entry->dst_port = dst_port;
    entry->payload_size = payload_size;
    entry->proto = proto;

    // Add the entry to the list with spinlock (to protect the list from concurrent access on usr get packets request)
    spin_lock(&pckt_lock);
    list_add_tail(&entry->list, &pckt_list);
    spin_unlock(&pckt_lock);



    return NF_ACCEPT;  // Let the packet continue normally
}


// init function (runs on module load)
static int __init sniffer_init(void) {
    pr_info("[sniffer] Module loaded.\n");

    // Create a Netlink socket
    struct netlink_kernel_cfg cfg = {// Netlink socket configuration
        .input = nl_recv_msg, 
    };   
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);// Creats the actual socket
    if (!nl_sk) {
        pr_err("sniffer: Failed to create Netlink socket\n");
        return -ENOMEM;
    }   
    pr_info("sniffer: Netlink socket created\n");
    


    // Netfilter hook initilaziation registration
    nfho.hook = packet_sniffer_hook;       // The function to run when a packet hits the hook
    nfho.hooknum = NF_INET_LOCAL_IN;    // Places the hook after basec proccessing, the skb header will be complete at this stage, wait for ipv4 packets
    nfho.pf = PF_INET;                     // IPv4 packets only
    nfho.priority = NF_IP_PRI_FIRST;       // Run before other hooks (highest priority)
    nf_register_net_hook(&init_net, &nfho); // Register the hook with netfilter (init_net refers to the default network namespace)

    
    return 0;
}

// exit function (runs on module unload)
static void __exit sniffer_exit(void) {
    // Free the list and its entries
    struct pckt_info *entry, *tmp;
    
    spin_lock(&pckt_lock);// Lock the list before iterating over it to prevent concurrent access
    // Iterate over the list and free each entry
    list_for_each_entry_safe(entry, tmp, &pckt_list, list) {
        list_del(&entry->list);
        kfree(entry);
    }
    spin_unlock(&pckt_lock);
    
    nf_unregister_net_hook(&init_net, &nfho); // Unregister the hook (if the hook is not unregistered, it will remain active even after the module is unloaded) 
    
    // Unregister the Netlink socket
    if (nl_sk) {
        netlink_kernel_release(nl_sk);
        pr_info("sniffer: Netlink socket released\n");
    }
    
    pr_info("[sniffer] Module unloaded.\n");
}

// Register the init and exit functions
module_init(sniffer_init);
module_exit(sniffer_exit);
