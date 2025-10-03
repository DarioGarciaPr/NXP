#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include "nxp_simtemp.h"
#include "nxp_simtemp_ioctl.h"

#define DEVICE_NAME "simtemp"

static struct simtemp_device *gdev;

/* === Helper functions === */
static void simtemp_push_sample(struct simtemp_device *dev, s32 temp_mC, bool alert)
{
    unsigned long flags;
    struct simtemp_sample_internal s;

    s.data.timestamp_ns = ktime_get_ns();
    s.data.temp_mC = temp_mC;
    s.data.flags = 1; // NEW_SAMPLE
    if (alert)
        s.data.flags |= (1 << 1);

    spin_lock_irqsave(&dev->ring_lock, flags);
    dev->ring[dev->head] = s;
    dev->head = (dev->head + 1) % SIMTEMP_RING_SIZE;
    if (dev->head == dev->tail) // overwrite
        dev->tail = (dev->tail + 1) % SIMTEMP_RING_SIZE;
    spin_unlock_irqrestore(&dev->ring_lock, flags);

    wake_up_interruptible(&dev->wq);
}

/* === Work handler === */
static void simtemp_work_handler(struct work_struct *work)
{
    struct simtemp_device *dev = gdev;
    s32 temp = 40000 + (prandom_u32() % 2000); // 40.0C to 42.0C
    bool alert = (temp >= dev->config.threshold_mC);

    dev->stats.updates++;
    if (alert)
        dev->stats.alerts++;

    simtemp_push_sample(dev, temp, alert);
}

/* === Timer callback === */
static enum hrtimer_restart simtemp_timer_cb(struct hrtimer *timer)
{
    struct simtemp_device *dev = gdev;
    schedule_work(&dev->work);
    hrtimer_forward_now(timer, ms_to_ktime(dev->config.sampling_ms));
    return HRTIMER_RESTART;
}

/* === File ops === */
static ssize_t simtemp_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct simtemp_device *dev = gdev;
    struct simtemp_sample_internal s;
    unsigned long flags;
    int ret;

    if (len < sizeof(struct simtemp_sample))
        return -EINVAL;

    if (dev->head == dev->tail) {
        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
        wait_event_interruptible(dev->wq, dev->head != dev->tail);
    }

    spin_lock_irqsave(&dev->ring_lock, flags);
    if (dev->head == dev->tail) {
        spin_unlock_irqrestore(&dev->ring_lock, flags);
        return -EAGAIN;
    }
    s = dev->ring[dev->tail];
    dev->tail = (dev->tail + 1) % SIMTEMP_RING_SIZE;
    spin_unlock_irqrestore(&dev->ring_lock, flags);

    ret = copy_to_user(buf, &s.data, sizeof(s.data));
    if (ret)
        return -EFAULT;

    return sizeof(s.data);
}

static __poll_t simtemp_poll(struct file *filp, poll_table *wait)
{
    struct simtemp_device *dev = gdev;
    __poll_t mask = 0;

    poll_wait(filp, &dev->wq, wait);

    if (dev->head != dev->tail)
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static long simtemp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct simtemp_device *dev = gdev;

    switch (cmd) {
    case SIMTEMP_IOC_RESET_STATS:
        dev->stats.updates = dev->stats.alerts = dev->stats.errors = 0;
        return 0;
    case SIMTEMP_IOC_GET_STATS: {
        struct simtemp_stats s = {
            .updates = dev->stats.updates,
            .alerts = dev->stats.alerts,
            .errors = dev->stats.errors,
        };
        if (copy_to_user((void __user *)arg, &s, sizeof(s)))
            return -EFAULT;
        return 0;
    }
    default:
        return -ENOTTY;
    }
}

static const struct file_operations simtemp_fops = {
    .owner          = THIS_MODULE,
    .read           = simtemp_read,
    .poll           = simtemp_poll,
    .unlocked_ioctl = simtemp_ioctl,
    .llseek         = no_llseek,
};

/* === Sysfs attrs === */
static ssize_t sampling_ms_show(struct device *devdev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", gdev->config.sampling_ms);
}
static ssize_t sampling_ms_store(struct device *devdev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long val;
    if (kstrtoul(buf, 10, &val))
        return -EINVAL;
    gdev->config.sampling_ms = val;
    return count;
}
static DEVICE_ATTR_RW(sampling_ms);

static ssize_t threshold_mC_show(struct device *devdev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gdev->config.threshold_mC);
}
static ssize_t threshold_mC_store(struct device *devdev, struct device_attribute *attr, const char *buf, size_t count)
{
    long val;
    if (kstrtol(buf, 10, &val))
        return -EINVAL;
    gdev->config.threshold_mC = val;
    return count;
}
static DEVICE_ATTR_RW(threshold_mC);

static ssize_t stats_show(struct device *devdev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "updates=%llu alerts=%llu errors=%llu\n",
                   gdev->stats.updates, gdev->stats.alerts, gdev->stats.errors);
}
static DEVICE_ATTR_RO(stats);

static struct attribute *simtemp_attrs[] = {
    &dev_attr_sampling_ms.attr,
    &dev_attr_threshold_mC.attr,
    &dev_attr_stats.attr,
    NULL,
};
ATTRIBUTE_GROUPS(simtemp);

/* === Platform driver === */
static int simtemp_probe(struct platform_device *pdev)
{
    int ret;
    struct simtemp_device *dev;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;
    gdev = dev;

    dev->config.sampling_ms = 100;
    dev->config.threshold_mC = 42000;
    dev->config.mode = 0;

    spin_lock_init(&dev->ring_lock);
    mutex_init(&dev->config_lock);
    init_waitqueue_head(&dev->wq);

    INIT_WORK(&dev->work, simtemp_work_handler);
    hrtimer_init(&dev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    dev->timer.function = simtemp_timer_cb;
    hrtimer_start(&dev->timer, ms_to_ktime(dev->config.sampling_ms), HRTIMER_MODE_REL);

    dev->miscdev.minor = MISC_DYNAMIC_MINOR;
    dev->miscdev.name  = DEVICE_NAME;
    dev->miscdev.fops  = &simtemp_fops;
    dev->miscdev.groups = simtemp_groups;

    ret = misc_register(&dev->miscdev);
    if (ret) {
        kfree(dev);
        return ret;
    }

    dev_info(&pdev->dev, "nxp_simtemp probed\n");
    return 0;
}

static int simtemp_remove(struct platform_device *pdev)
{
    struct simtemp_device *dev = gdev;
    hrtimer_cancel(&dev->timer);
    cancel_work_sync(&dev->work);
    misc_deregister(&dev->miscdev);
    kfree(dev);
    return 0;
}

static const struct of_device_id simtemp_of_match[] = {
    { .compatible = "nxp,simtemp", },
    { },
};
MODULE_DEVICE_TABLE(of, simtemp_of_match);

static struct platform_driver simtemp_driver = {
    .driver = {
        .name = "nxp_simtemp",
        .of_match_table = simtemp_of_match,
    },
    .probe = simtemp_probe,
    .remove = simtemp_remove,
};

module_platform_driver(simtemp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Garcia");
MODULE_DESCRIPTION("NXP Simulated Temperature Sensor");

