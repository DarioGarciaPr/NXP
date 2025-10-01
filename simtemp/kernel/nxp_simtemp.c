#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/types.h>

#define BUF_SIZE 128

struct simtemp_sample {
    __u64 timestamp_ns;
    __s32 temp_mC;
    __u32 flags;
} __attribute__((packed));

struct simtemp_dev {
    struct simtemp_sample buffer[BUF_SIZE];
    int head;
    int tail;
    struct mutex lock;
    struct miscdevice miscdev;
    struct timer_list timer;
};

static struct simtemp_dev *simdev;

static void simtemp_timer_cb(struct timer_list *t) {
    struct simtemp_dev *dev = from_timer(dev, t, timer);
    struct simtemp_sample sample;

    sample.timestamp_ns = ktime_get_ns();
    sample.temp_mC = 42000; // ejemplo fijo
    sample.flags = 1; // NEW_SAMPLE

    mutex_lock(&dev->lock);
    dev->buffer[dev->head] = sample;
    dev->head = (dev->head + 1) % BUF_SIZE;
    if(dev->head == dev->tail)
        dev->tail = (dev->tail + 1) % BUF_SIZE; // descarta el más viejo
    mutex_unlock(&dev->lock);

    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(100)); // 100ms
}

static ssize_t simtemp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    struct simtemp_sample sample;
    ssize_t ret = 0;

    mutex_lock(&simdev->lock);
    if(simdev->head == simdev->tail) {
        ret = 0; // buffer vacío
        goto out;
    }
    sample = simdev->buffer[simdev->tail];
    simdev->tail = (simdev->tail + 1) % BUF_SIZE;
out:
    mutex_unlock(&simdev->lock);

    if(ret == 0 && copy_to_user(buf, &sample, sizeof(sample)))
        return -EFAULT;

    return sizeof(sample);
}

static const struct file_operations simtemp_fops = {
    .owner = THIS_MODULE,
    .read  = simtemp_read,
};

static int __init nxp_simtemp_init(void) {
    simdev = kzalloc(sizeof(*simdev), GFP_KERNEL);
    if(!simdev)
        return -ENOMEM;

    mutex_init(&simdev->lock);

    simdev->miscdev.minor = MISC_DYNAMIC_MINOR;
    simdev->miscdev.name = "simtemp";
    simdev->miscdev.fops = &simtemp_fops;
    misc_register(&simdev->miscdev);

    timer_setup(&simdev->timer, simtemp_timer_cb, 0);
    mod_timer(&simdev->timer, jiffies + msecs_to_jiffies(100));

    printk(KERN_INFO "nxp_simtemp: module loaded\n");
    return 0;
}

static void __exit nxp_simtemp_exit(void) {
    del_timer_sync(&simdev->timer);
    misc_deregister(&simdev->miscdev);
    kfree(simdev);
    printk(KERN_INFO "nxp_simtemp: module unloaded\n");
}

module_init(nxp_simtemp_init);
module_exit(nxp_simtemp_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Darío García");
MODULE_DESCRIPTION("NXP Simulated Temperature Sensor with miscdevice skeleton and ring buffer");

