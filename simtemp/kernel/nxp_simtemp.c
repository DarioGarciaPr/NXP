#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/random.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#include "nxp_simtemp.h"

// ======================
// Variables globales
// ======================
static struct simtemp_sample latest_sample;
static unsigned int sampling_ms = 100;   // periodo default 100 ms
static int32_t threshold_mC = 45000;     // umbral default = 45°C
static char mode[16] = "normal";

static struct timer_list simtemp_timer;
static struct workqueue_struct *simtemp_wq;
static struct work_struct simtemp_work;

static struct mutex simtemp_lock;

static struct kobject *simtemp_kobj;

// ======================
// Funciones auxiliares
// ======================

static void generate_sample(void)
{
    mutex_lock(&simtemp_lock);

    latest_sample.timestamp_ns = ktime_get_ns();
    latest_sample.temp_mC = 40000 + (get_random_u32() % 10000); // 40–50°C
    latest_sample.flags = 0x1; // siempre NEW_SAMPLE

    if (latest_sample.temp_mC >= threshold_mC)
        latest_sample.flags |= 0x2; // THRESHOLD_CROSSED

    mutex_unlock(&simtemp_lock);
}

static void work_handler(struct work_struct *work)
{
    generate_sample();
    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(sampling_ms));
}

static void timer_callback(struct timer_list *t)
{
    queue_work(simtemp_wq, &simtemp_work);
}

// ======================
// Operaciones de archivo
// ======================

static ssize_t simtemp_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    if (len < sizeof(latest_sample))
        return -EINVAL;

    mutex_lock(&simtemp_lock);
    if (copy_to_user(buf, &latest_sample, sizeof(latest_sample))) {
        mutex_unlock(&simtemp_lock);
        return -EFAULT;
    }
    mutex_unlock(&simtemp_lock);

    return sizeof(latest_sample);
}

static const struct file_operations simtemp_fops = {
    .owner = THIS_MODULE,
    .read  = simtemp_read,
};

// ======================
// Sysfs: atributos
// ======================

static ssize_t sampling_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", sampling_ms);
}

static ssize_t sampling_store(struct kobject *kobj, struct kobj_attribute *attr,
                              const char *buf, size_t count)
{
    unsigned int val;
    if (kstrtouint(buf, 10, &val))
        return -EINVAL;

    mutex_lock(&simtemp_lock);
    sampling_ms = val;
    mutex_unlock(&simtemp_lock);
    return count;
}

static ssize_t threshold_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", threshold_mC);
}

static ssize_t threshold_store(struct kobject *kobj, struct kobj_attribute *attr,
                               const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val))
        return -EINVAL;

    mutex_lock(&simtemp_lock);
    threshold_mC = val;
    mutex_unlock(&simtemp_lock);
    return count;
}

static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", mode);
}

static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    mutex_lock(&simtemp_lock);
    strncpy(mode, buf, sizeof(mode));
    mode[sizeof(mode)-1] = '\0';
    mutex_unlock(&simtemp_lock);
    return count;
}

static struct kobj_attribute sampling_attr = __ATTR(sampling_ms, 0664, sampling_show, sampling_store);
static struct kobj_attribute threshold_attr = __ATTR(threshold_mC, 0664, threshold_show, threshold_store);
static struct kobj_attribute mode_attr = __ATTR(mode, 0664, mode_show, mode_store);

static struct attribute *simtemp_attrs[] = {
    &sampling_attr.attr,
    &threshold_attr.attr,
    &mode_attr.attr,
    NULL,
};

static struct attribute_group simtemp_group = {
    .attrs = simtemp_attrs,
};

// ======================
// Dispositivo misc
// ======================

static struct miscdevice simtemp_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "nxp_simtemp",
    .fops  = &simtemp_fops,
};

// ======================
// Init y Exit
// ======================

static int __init simtemp_init(void)
{
    int ret;

    mutex_init(&simtemp_lock);

    ret = misc_register(&simtemp_dev);
    if (ret)
        return ret;

    simtemp_kobj = kobject_create_and_add("simtemp", kernel_kobj);
    if (!simtemp_kobj)
        return -ENOMEM;

    if (sysfs_create_group(simtemp_kobj, &simtemp_group)) {
        kobject_put(simtemp_kobj);
        misc_deregister(&simtemp_dev);
        return -EINVAL;
    }

    simtemp_wq = create_singlethread_workqueue("simtemp_wq");
    if (!simtemp_wq) {
        sysfs_remove_group(simtemp_kobj, &simtemp_group);
        kobject_put(simtemp_kobj);
        misc_deregister(&simtemp_dev);
        return -ENOMEM;
    }

    INIT_WORK(&simtemp_work, work_handler);
    timer_setup(&simtemp_timer, timer_callback, 0);
    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(sampling_ms));

    pr_info("nxp_simtemp: module loaded\n");
    return 0;
}

static void __exit simtemp_exit(void)
{
    del_timer_sync(&simtemp_timer);
    flush_workqueue(simtemp_wq);
    destroy_workqueue(simtemp_wq);

    if (simtemp_kobj) {
        sysfs_remove_group(simtemp_kobj, &simtemp_group);
        kobject_put(simtemp_kobj);
    }

    misc_deregister(&simtemp_dev);

    pr_info("nxp_simtemp: module unloaded\n");
}

module_init(simtemp_init);
module_exit(simtemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tu Nombre");
MODULE_DESCRIPTION("NXP Simulated Temperature Sensor Driver");

