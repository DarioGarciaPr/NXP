#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init nxp_simtemp_init(void) {
    printk(KERN_INFO "nxp_simtemp: module loaded\n");
    return 0;
}

static void __exit nxp_simtemp_exit(void) {
    printk(KERN_INFO "nxp_simtemp: module unloaded\n");
}

module_init(nxp_simtemp_init);
module_exit(nxp_simtemp_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Garcia");
MODULE_DESCRIPTION("NXP Simulated Temperature Sensor Skeleton");
