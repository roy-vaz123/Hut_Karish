#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x92997ed8, "_printk" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x1343e6f5, "init_net" },
	{ 0xbd245904, "__netlink_kernel_create" },
	{ 0x84bb745e, "nf_register_net_hook" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x66e1119e, "nf_unregister_net_hook" },
	{ 0x6ea2152, "netlink_kernel_release" },
	{ 0xdc324703, "__alloc_skb" },
	{ 0x6de57620, "kfree_skb_reason" },
	{ 0x461f148, "__nlmsg_put" },
	{ 0x3d6af433, "netlink_unicast" },
	{ 0x8d5931c0, "kmalloc_caches" },
	{ 0xca078420, "kmalloc_trace" },
	{ 0x37a0cba, "kfree" },
	{ 0x82583a65, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "E7775B34A4760C3E66CB1ED");
