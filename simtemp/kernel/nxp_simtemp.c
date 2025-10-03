#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include "nxp_simtemp_ioctl.h"  // Defines NXPSIM_IOCTL_SET_THRESHOLD, NXPSIM_IOCTL_SET_SAMPLING_MS

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Darío García");
MODULE_DESCRIPTION("NXP temperature sensor simulator with DT and poll");

// Structure for temperature sample data
struct temp_sample {
    int temp_mC;   // Temperature in milliCelsius
    int flags;
};

// Global variables
static struct temp_sample latest_sample;
static int simtemp_threshold = 30000;      // Default threshold: 30°C
static int simtemp_sampling_ms = 500;      // Default sampling interval (ms)

static struct timer_list simtemp_timer;
static struct workqueue_struct *simtemp_wq;
static struct work_struct simtemp_work;

static struct platform_device *simtemp_pdev;

static DECLARE_WAIT_QUEUE_HEAD(simtemp_wq_read);
static bool data_ready = false;

// Function to generate a random sample
static void simtemp_generate_sample(struct work_struct *work)
{
    latest_sample.temp_mC = 40000 + (get_random_u32() % 10000); // Range: 40°C to 50°C
    latest_sample.flags = (latest_sample.temp_mC >= simtemp_threshold*1000) ? 1 : 0;

    data_ready = true;
    wake_up_interruptible(&simtemp_wq_read);

    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(simtemp_sampling_ms));
}

// Timer callback
static void simtemp_timer_cb(struct timer_list *t)
{
    queue_work(simtemp_wq, &simtemp_work);
}

// Read function
static ssize_t simtemp_read(struct file *file, char __user *buf,
                             size_t count, loff_t *ppos)
{
    char tmp[128];
    int len;

    int temp_int = latest_sample.temp_mC / 1000;
    int temp_frac = latest_sample.temp_mC % 1000;

    len = snprintf(tmp, sizeof(tmp), "Temp: %d.%03d °C | flags=0x%x\n",
                   temp_int, temp_frac, latest_sample.flags);

    if (*ppos >= len) {
        *ppos = 0;
    }

    if (count > len) count = len;

    if (copy_to_user(buf, tmp, count))
        return -EFAULT;

    *ppos += count;
    data_ready = false; // reset flag

    return count;
}

// IOCTL function
static long simtemp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int value;

    switch (cmd) {
        case NXPSIM_IOCTL_SET_THRESHOLD:
            if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            simtemp_threshold = value;
            pr_info("Threshold set to %d\n", simtemp_threshold);
            break;

        case NXPSIM_IOCTL_SET_SAMPLING_MS:
            if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            simtemp_sampling_ms = value;
            pr_info("Sampling interval set to %d ms\n", simtemp_sampling_ms);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

// Poll function
static unsigned int simtemp_poll(struct file *file, poll_table *wait)
{
    poll_wait(file, &simtemp_wq_read, wait);
    return data_ready ? POLLIN | POLLRDNORM : 0;
}

// File operations
static const struct file_operations simtemp_fops = {
    .owner = THIS_MODULE,
    .read = simtemp_read,
    .unlocked_ioctl = simtemp_ioctl,
    .poll = simtemp_poll,
    .llseek = noop_llseek,
};

// Misc device
static struct miscdevice simtemp_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "nxp_simtemp",
    .fops  = &simtemp_fops,
};

static const struct of_device_id simtemp_of_match[] = {
    { .compatible = "nxp,simtemp", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, simtemp_of_match);

// Probe function
static int simtemp_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;

    if (np) {
        pr_info("Device tree node found\n");
        of_property_read_u32(np, "threshold-mC", &simtemp_threshold);
        pr_info("Threshold from DT: %d\n", simtemp_threshold);
        of_property_read_u32(np, "sampling-ms", &simtemp_sampling_ms);
        pr_info("Sampling interval from DT: %d ms\n", simtemp_sampling_ms);
    }

    simtemp_wq = create_singlethread_workqueue("simtemp_wq");
    INIT_WORK(&simtemp_work, simtemp_generate_sample);
    timer_setup(&simtemp_timer, simtemp_timer_cb, 0);
    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(simtemp_sampling_ms));

    return misc_register(&simtemp_dev);
}

// Remove function
static void simtemp_remove(struct platform_device *pdev)
{
    del_timer_sync(&simtemp_timer);
    cancel_work_sync(&simtemp_work);
    destroy_workqueue(simtemp_wq);
    misc_deregister(&simtemp_dev);
}

static struct platform_driver simtemp_driver = {
    .driver = {
        .name = "nxp_simtemp",
        .of_match_table = simtemp_of_match,
    },
    .probe = simtemp_probe,
    .remove = simtemp_remove,
};

// Module init
static int __init simtemp_init(void)
{
    int ret;

    // Register the device manually
    simtemp_pdev = platform_device_register_simple("nxp_simtemp", -1, NULL, 0);
    if (IS_ERR(simtemp_pdev))
        return PTR_ERR(simtemp_pdev);

    // Register the driver
    ret = platform_driver_register(&simtemp_driver);
    if (ret) {
        platform_device_unregister(simtemp_pdev);
        return ret;
    }

    return 0;
}

// Module exit
static void __exit simtemp_exit(void)
{
    platform_driver_unregister(&simtemp_driver);
    platform_device_unregister(simtemp_pdev);
}

module_init(simtemp_init);
module_exit(simtemp_exit);

