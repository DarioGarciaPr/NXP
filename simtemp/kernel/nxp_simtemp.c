#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/random.h>

#define RING_SIZE 128

#define FLAG_NEW_SAMPLE        (1 << 0)
#define FLAG_THRESHOLD_CROSSED (1 << 1)

struct simtemp_sample {
    __u64 timestamp_ns;
    __s32 temp_mC;
    __u32 flags;
} __attribute__((packed));

struct simtemp_dev {
    struct miscdevice miscdev;
    struct simtemp_sample ring[RING_SIZE];
    int head;
    int tail;
    int threshold_mC;
    int sampling_ms;
    struct hrtimer timer;
    struct work_struct work;
    wait_queue_head_t wq;
};

static struct simtemp_dev *simdev;

/* Workqueue handler */
static void simtemp_work_handler(struct work_struct *work)
{
    struct simtemp_sample *s;
    s32 temp;

    temp = 40000 + (get_random_u32() % 2000); // 40.0C a 42.0C
    s = &simdev->ring[simdev->head];
    s->timestamp_ns = ktime_get_ns();
    s->temp_mC = temp;
    s->flags = FLAG_NEW_SAMPLE;
    if (temp >= simdev->threshold_mC)
        s->flags |= FLAG_THRESHOLD_CROSSED;

    simdev->head = (simdev->head + 1) % RING_SIZE;
    wake_up_interruptible(&simdev->wq);
}

/* Timer callback */
static enum hrtimer_restart simtemp_timer_cb(struct hrtimer *timer)
{
    schedule_work(&simdev->work);
    hrtimer_forward_now(timer, ms_to_ktime(simdev->sampling_ms));
    return HRTIMER_RESTART;
}

/* Sysfs attributes */
static ssize_t threshold_mC_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", simdev->threshold_mC);
}

static ssize_t threshold_mC_store(struct device *dev,
                                  struct device_attribute *attr, const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;
    simdev->threshold_mC = val;
    pr_info("Threshold set to %d\n", val);
    return count;
}

static ssize_t sampling_ms_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", simdev->sampling_ms);
}

static ssize_t sampling_ms_store(struct device *dev,
                                 struct device_attribute *attr, const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;
    simdev->sampling_ms = val;
    pr_info("Sampling interval set to %d ms\n", val);
    return count;
}

static DEVICE_ATTR_RW(threshold_mC);
static DEVICE_ATTR_RW(sampling_ms);

static struct attribute *simtemp_attrs[] = {
    &dev_attr_threshold_mC.attr,
    &dev_attr_sampling_ms.attr,
    NULL
};
static struct attribute_group simtemp_group = {
    .attrs = simtemp_attrs,
};

/* File operations */
static int simtemp_open(struct inode *inode, struct file *file) { return 0; }
static int simtemp_release(struct inode *inode, struct file *file) { return 0; }

static ssize_t simtemp_read(struct file *file, char __user *buf,
                            size_t count, loff_t *ppos)
{
    struct simtemp_sample *s;
    unsigned int tail = simdev->tail;

    if (tail == simdev->head)
        return 0;

    s = &simdev->ring[tail];
    if (copy_to_user(buf, s, sizeof(*s)))
        return -EFAULT;

    simdev->tail = (tail + 1) % RING_SIZE;
    return sizeof(*s);
}

static unsigned int simtemp_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &simdev->wq, wait);

    if (simdev->head != simdev->tail)
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static const struct file_operations simtemp_fops = {
    .owner = THIS_MODULE,
    .open = simtemp_open,
    .release = simtemp_release,
    .read = simtemp_read,
    .poll = simtemp_poll,
};

/* Probe / Remove */
static int simtemp_probe(struct platform_device *pdev)
{
    int ret;

    simdev = kzalloc(sizeof(*simdev), GFP_KERNEL);
    if (!simdev)
        return -ENOMEM;

    simdev->miscdev.minor = MISC_DYNAMIC_MINOR;
    simdev->miscdev.name  = "simtemp";
    simdev->miscdev.fops  = &simtemp_fops;

    ret = misc_register(&simdev->miscdev);
    if (ret) {
        kfree(simdev);
        return ret;
    }

    init_waitqueue_head(&simdev->wq);
    INIT_WORK(&simdev->work, simtemp_work_handler);

    simdev->sampling_ms = 1000;
    simdev->threshold_mC = 45000;
    hrtimer_init(&simdev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    simdev->timer.function = simtemp_timer_cb;
    hrtimer_start(&simdev->timer, ms_to_ktime(simdev->sampling_ms), HRTIMER_MODE_REL);

    ret = sysfs_create_group(&simdev->miscdev.this_device->kobj, &simtemp_group);
    if (ret) {
        misc_deregister(&simdev->miscdev);
        kfree(simdev);
        return ret;
    }

    pr_info("nxp_simtemp probe successful\n");
    return 0;
}

static void simtemp_remove(struct platform_device *pdev)
{
    hrtimer_cancel(&simdev->timer);
    cancel_work_sync(&simdev->work);
    sysfs_remove_group(&simdev->miscdev.this_device->kobj, &simtemp_group);
    misc_deregister(&simdev->miscdev);
    kfree(simdev);
}

static const struct of_device_id simtemp_of_match[] = {
    { .compatible = "nxp,simtemp", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, simtemp_of_match);

static struct platform_driver simtemp_driver = {
    .probe  = simtemp_probe,
    .remove = simtemp_remove,
    .driver = {
        .name = "nxp_simtemp",
        .of_match_table = simtemp_of_match,
    },
};

/* Platform device para demo en PC/Linux */
static struct platform_device *simtemp_pdev;

static int __init simtemp_init(void)
{
    int ret;

    simtemp_pdev = platform_device_register_simple("nxp_simtemp", -1, NULL, 0);
    if (IS_ERR(simtemp_pdev))
        return PTR_ERR(simtemp_pdev);

    ret = platform_driver_register(&simtemp_driver);
    if (ret) {
        platform_device_unregister(simtemp_pdev);
        return ret;
    }

    pr_info("nxp_simtemp module loaded (PC/Linux demo)\n");
    return 0;
}

static void __exit simtemp_exit(void)
{
    platform_driver_unregister(&simtemp_driver);
    platform_device_unregister(simtemp_pdev);
    pr_info("nxp_simtemp module unloaded\n");
}

module_init(simtemp_init);
module_exit(simtemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Darío García");
MODULE_DESCRIPTION("NXP Simulated Temperature Kernel Module");

