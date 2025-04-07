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
// needed for kmalloc and kfree
#include <linux/slab.h>  
// Netlink socket
#include <net/sock.h>
#include <linux/netlink.h>
// Project netlink config
#include "../shared/config/NetLinkConfig.h" 



// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roy");
MODULE_DESCRIPTION("Basic Packet Sniffer Kernel Module");
MODULE_VERSION("0.1");


static struct nf_hook_ops nfho;  // Netfilter hook options struct
struct sock *nl_sk = NULL; // Netlink socket struct, used to send messages to user space

// Netlink clients subscribed to the kernel module, and thir PIDs
static int packet_hunter_subscribed = 0;
static int daemon_subscribed = 0;
static u32 packet_hunter_pid = 0;
static u32 daemon_pid = 0;




// Sends a single pckt_info struct to a client using Netlink
static void send_packet_info_to_user(u32 pid, const struct pckt_info *msg) {
    
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;

    // Allocate a new skb for the Netlink message
    nl_skb = nlmsg_new(sizeof(*msg), GFP_ATOMIC);
    if (!nl_skb) {
        pr_info("[sniffer] Failed to allocate skb for Netlink message\n");
        return;
    }

    // Prepare the Netlink message header and payload
    nlh = nlmsg_put(nl_skb, 0, 0, NLMSG_DONE, sizeof(*msg), 0);
    if (!nlh) {
        pr_info("[sniffer] Failed to create Netlink header\n");
        kfree_skb(nl_skb);
        return;
    }

    // Copy the packet info into the message payload
    memcpy(nlmsg_data(nlh), msg, sizeof(*msg));

    // Send the Netlink message to the user process
    int res = netlink_unicast(nl_sk, nl_skb, pid, MSG_DONTWAIT);
    if (res < 0) {
        pr_info("[sniffer] Failed to send Netlink message, error: %d\n", res);
        // nl_skb is freed automatically on error
    }
}

// Netlink Receive function (called when a message is received from user space) to subscribe/unsubscribe
static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    char *user_msg;
    
    // Check if the skb is NULL
    if (!skb) {
        pr_err("sniffer: Received NULL skb\n");
        return;
    }

    // Extract the Netlink message header
    nlh = (struct nlmsghdr *)skb->data;

    // Extract the actual message data
    user_msg = (char *)nlmsg_data(nlh);

    pr_info("sniffer: received netlink message: %s\n", user_msg);

    // Subscribe/unsubscribe netlink cliets according to messages (to the sender proccess)
    if (strcmp(user_msg, "packet_hunter_subscribe") == 0) {
        packet_hunter_subscribed = 1;
        packet_hunter_pid = nlh->nlmsg_pid; // Get the PID of the sender process
        pr_info("sniffer: packet_hunter_subscribed to packet notifications from PID: %u\n", packet_hunter_pid);
        return;

    }
    if(strcmp(user_msg, "packet_hunter_unsubscribe") == 0){
        packet_hunter_subscribed = 0;
        packet_hunter_pid = 0;
        pr_info("sniffer: packet_hunter unsubscribed from packet notifications\n");
        return;   
    }
    if (strcmp(user_msg, "daemon_subscribe") == 0) {
        daemon_subscribed = 1;
        daemon_pid = nlh->nlmsg_pid; // Get the PID of the sender process
        pr_info("sniffer: daemon_subscribed to packet notifications from PID: %u\n", daemon_pid);
        return;

    }
    if(strcmp(user_msg, "daemon_unsubscribe") == 0){
        daemon_subscribed = 0;
        daemon_pid = 0;
        pr_info("sniffer: daemon nsubscribed from packet notifications\n");
        return;

    }
    pr_info("sniffer: Unknown command: %s\n", user_msg);
    
}

// Creat pckt info struct to send based of data from hook
static struct pckt_info* create_message( u32 src_ip, u32 dst_ip, u16 src_port, u16 dst_port, char proto) {
    struct pckt_info *msg = kmalloc(sizeof(*msg), GFP_ATOMIC);
    if (!msg) {
        pr_err("sniffer: Failed to allocate memory for packet info\n");
        return NULL;
    }

    // Fill the packet info struct with the packet's info
    msg->src_ip = src_ip;
    msg->dst_ip = dst_ip;
    msg->src_port = src_port;
    msg->dst_port = dst_port;
    msg->proto = proto;

    return msg;
    
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
    struct pckt_info *msg; // The message to send to user space
    
    // Check if the skb is NULL or too short (packets comes as sk_buff struct, skb = the packet)
    if (!skb || skb->len < sizeof(struct iphdr)) {
        pr_info("[sniffer] Packet too short or skb is NULL. \n");
        return NF_ACCEPT;
    }
    
    iph = ip_hdr(skb);
    if (!iph)// If failed to retrive ip header from packet (non ip packet or corrupted) using ip_hdr() macro
        return NF_ACCEPT;
    
    // Get the source and destination IP addresses
    src_ip = iph->saddr; // Convert to host byte order
    dst_ip = iph->daddr;



    // Fill the packet info struct members according to the packet protocol (TCP or UDP)
    if (iph->protocol == IPPROTO_TCP) {// TCP
        tcph = tcp_hdr(skb);
        src_port = ntohs(tcph->source);
        dst_port = ntohs(tcph->dest);
        proto = PROTO_TCP;
        payload_size = skb->len - skb_transport_offset(skb) - (tcph->doff * 4);  // skb_transport_offset(skb) to safely locate the start of TCP/UDP headers
    
    
    } else if (iph->protocol == IPPROTO_UDP) { // UDP
        udph = udp_hdr(skb);
        src_port = ntohs(udph->source);
        dst_port = ntohs(udph->dest);
        proto = PROTO_UDP;
        payload_size = skb->len - skb_transport_offset(skb) - sizeof(struct udphdr); 
    
    
    } else { // Our module only supports TCP and UDP packets
        pr_info("[sniffer] Unsupported protocol: %d \n", iph->protocol);
        return NF_ACCEPT;
    }
                
    
    // pr_info the packet info, for debugging
    pr_info("[sniffer] Packet type %c Src IP: %pI4, Dst IP: %pI4, Src Port: %u, Dst Port: %u, Payload: %u bytes \n", proto, &src_ip, &dst_ip, src_port, dst_port, payload_size);
 
    
     // if the daemon is subscribed, create and send the packet's info to the daemon pid
     if( daemon_subscribed && daemon_pid != 0) {           
        msg = create_message(src_ip, dst_ip, src_port, dst_port, proto);
        if (!msg)
            return NF_ACCEPT;
        
        // Send the packet info to the user process
        send_packet_info_to_user(daemon_pid, msg);
        kfree(msg);
        pr_info("[sniffer] Packet info sent to user process PID: %u\n", daemon_pid);
    }
    
    // If the packet_hunter is subscribed, create and send the packet's info to the packet_hunter pid
    if (packet_hunter_subscribed && packet_hunter_pid != 0) {           
        msg = create_message(src_ip, dst_ip, src_port, dst_port, proto);
        if (!msg)
            return NF_ACCEPT;
        
        // Send the packet info to the user process
        send_packet_info_to_user(packet_hunter_pid, msg);
        kfree(msg);
        pr_info("[sniffer] Packet info sent to user process PID: %u\n", packet_hunter_pid);
    }
    

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
   
    // Unregister the hook (if the hook is not unregistered, it will remain active even after the module is unloaded) 
    nf_unregister_net_hook(&init_net, &nfho); 

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
