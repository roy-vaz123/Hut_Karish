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

// init function (runs on module load)
static int __init sniffer_init(void) {
    printk(KERN_INFO "[sniffer] Module loaded.\n");
    return 0;
}

// exit function (runs on module unload)
static void __exit sniffer_exit(void) {
    printk(KERN_INFO "[sniffer] Module unloaded.\n");
}

// Register the init and exit functions
module_init(sniffer_init);
module_exit(sniffer_exit);
