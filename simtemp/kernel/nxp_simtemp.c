#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Darío García");
MODULE_DESCRIPTION("MVP nxp_simtemp safe version");

struct simtemp_sample {
    u64 timestamp_ns;
    s32 temp_mC;
    u32 flags;
} __attribute__((packed));

#define FLAG_NEW_SAMPLE       0x1
#define FLAG_THRESHOLD_CROSS  0x2

static struct simtemp_sample latest_sample;
static struct timer_list simtemp_timer;
static wait_queue_head_t simtemp_wq;
static spinlock_t sample_lock;
static struct miscdevice simtemp_misc;

static unsigned int sampling_ms = 100;
static int threshold_mC = 45000;

static void timer_callback(struct timer_list *t)
{
    unsigned long flags;
    u32 alert = 0;

    latest_sample.timestamp_ns = ktime_get_ns();
    latest_sample.temp_mC = 40000 + (prandom_u32() % 10000); // 40-50°C

    if (latest_sample.temp_mC >= threshold_mC)
        alert = FLAG_THRESHOLD_CROSS;

    spin_lock_irqsave(&sample_lock, flags);
    latest_sample.flags = FLAG_NEW_SAMPLE | alert;
    spin_unlock_irqrestore(&sample_lock, flags);

    wake_up_interruptible(&simtemp_wq);

    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(sampling_ms));
}

static ssize_t simtemp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct simtemp_sample sample_copy;
    int ret;

    if (count < sizeof(sample_copy))
        return -EINVAL;

    if (file->f_flags & O_NONBLOCK) {
        // copiar último sample
        spin_lock(&sample_lock);
        sample_copy = latest_sample;
        spin_unlock(&sample_lock);
    } else {
        // espera un sample
        ret = wait_event_interruptible(simtemp_wq, latest_sample.flags & FLAG_NEW_SAMPLE);
        if (ret)
            return ret;
        spin_lock(&sample_lock);
        sample_copy = latest_sample;
        spin_unlock(&sample_lock);
    }

    // limpiar NEW_SAMPLE
    spin_lock(&sample_lock);
    latest_sample.flags &= ~FLAG_NEW_SAMPLE;
    spin_unlock(&sample_lock);

    if (copy_to_user(buf, &sample_copy, sizeof(sample_copy)))
        return -EFAULT;

    return sizeof(sample_copy);
}

static unsigned int simtemp_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &simtemp_wq, wait);

    if (latest_sample.flags & FLAG_NEW_SAMPLE)
        mask |= POLLIN | POLLRDNORM;
    if (latest_sample.flags & FLAG_THRESHOLD_CROSS)
        mask |= POLLPRI;

    return mask;
}

static const struct file_operations simtemp_fops = {
    .owner = THIS_MODULE,
    .read = simtemp_read,
    .poll = simtemp_poll,
};

static int __init simtemp_init(void)
{
    int ret;

    spin_lock_init(&sample_lock);
    init_waitqueue_head(&simtemp_wq);

    ret = misc_register(&simtemp_misc);
    if (ret)
        return ret;

    timer_setup(&simtemp_timer, timer_callback, 0);
    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(sampling_ms));

    pr_info("nxp_simtemp loaded\n");
    return 0;
}

static void __exit simtemp_exit(void)
{
    del_timer_sync(&simtemp_timer);
    misc_deregister(&simtemp_misc);
    pr_info("nxp_simtemp unloaded\n");
}

static struct miscdevice simtemp_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "simtemp",
    .fops = &simtemp_fops,
};

module_init(simtemp_init);
module_exit(simtemp_exit);