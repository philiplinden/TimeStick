#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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


MODULE_INFO(depends, "mii,ptp");

MODULE_ALIAS("usb:v0B95p1790d0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p178Ad0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p178Ad00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0DF6p0072d0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0DF6p0072d00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v17EFp304Bd0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v17EFp304Bd00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0930p0A13d0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0930p0A13d00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v04E8pA100d0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v04E8pA100d00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v2001p4A00d0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v2001p4A00d00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0711p0179d0100dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0711p0179d00*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d0300dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d0[0-2]*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d0200dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d0[0-1]*dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d0400dc*dsc*dp*ic*isc*ip*in*");
MODULE_ALIAS("usb:v0B95p1790d0[0-3]*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "3822592555F9D07D13916E5");
