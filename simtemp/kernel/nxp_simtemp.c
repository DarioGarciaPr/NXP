#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/random.h>

#include "nxp_simtemp_ioctl.h"  // Define tus IOCTLs: NXPSIM_IOCTL_SET_THRESHOLD, NXPSIM_IOCTL_SET_SAMPLING_MS

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Darío García");
MODULE_DESCRIPTION("Simulador de sensor de temperatura NXP");

// Estructura para los datos de la muestra
struct temp_sample {
    int temp_mC;   // Temperatura en milicelsius
    int flags;
};

// Variables globales del módulo
static struct temp_sample latest_sample;
static int simtemp_threshold = 45000;      // Umbral por defecto 45°C
static int simtemp_sampling_ms = 500;      // Intervalo de muestreo por defecto (ms)

static struct timer_list simtemp_timer;
static struct workqueue_struct *simtemp_wq;
static struct work_struct simtemp_work;

// Función para generar una muestra aleatoria
static void simtemp_generate_sample(struct work_struct *work)
{
    latest_sample.temp_mC = 40000 + (get_random_u32() % 10000); // 40°C a 50°C
    latest_sample.flags = (latest_sample.temp_mC >= simtemp_threshold) ? 1 : 0;

    // Reprogramar timer
    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(simtemp_sampling_ms));
}

// Timer callback
static void simtemp_timer_cb(struct timer_list *t)
{
    queue_work(simtemp_wq, &simtemp_work);
}

// Función read
static ssize_t simtemp_read(struct file *file, char __user *buf,
                             size_t count, loff_t *ppos)
{
    char tmp[128];
    int len;

    // latest_sample.temp_mC está en miligrados (int)
    int temp_int = latest_sample.temp_mC / 1000;       // parte entera
    int temp_frac = latest_sample.temp_mC % 1000;      // parte decimal

    len = snprintf(tmp, sizeof(tmp), "Temp: %d.%03d °C | flags=0x%x\n",
                   temp_int, temp_frac,
                   latest_sample.flags);

    if (*ppos >= len) {
        *ppos = 0; // reset offset
    }

    if (count > len) count = len;

    if (copy_to_user(buf, tmp, count))
        return -EFAULT;

    *ppos += count;
    return count;
}
// Función ioctl
static long simtemp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int value;

    switch (cmd) {
        case NXPSIM_IOCTL_SET_THRESHOLD:
            if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            simtemp_threshold = value;
            pr_info("Threshold seteado a %d\n", simtemp_threshold);
            break;

        case NXPSIM_IOCTL_SET_SAMPLING_MS:
            if (copy_from_user(&value, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            simtemp_sampling_ms = value;
            pr_info("Sampling seteado a %d ms\n", simtemp_sampling_ms);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

// File operations
static const struct file_operations simtemp_fops = {
    .owner = THIS_MODULE,
    .read = simtemp_read,
    .unlocked_ioctl = simtemp_ioctl,
    .llseek = noop_llseek,
};

// Misc device
static struct miscdevice simtemp_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "nxp_simtemp",
    .fops  = &simtemp_fops,
};

// Init del módulo
static int __init simtemp_init(void)
{
    int ret;

    simtemp_wq = create_singlethread_workqueue("simtemp_wq");
    if (!simtemp_wq)
        return -ENOMEM;

    INIT_WORK(&simtemp_work, simtemp_generate_sample);
    timer_setup(&simtemp_timer, simtemp_timer_cb, 0);
    mod_timer(&simtemp_timer, jiffies + msecs_to_jiffies(simtemp_sampling_ms));

    ret = misc_register(&simtemp_dev);
    if (ret) {
        destroy_workqueue(simtemp_wq);
        return ret;
    }

    pr_info("nxp_simtemp driver loaded\n");
    return 0;
}

// Exit del módulo
static void __exit simtemp_exit(void)
{
    del_timer_sync(&simtemp_timer);
    cancel_work_sync(&simtemp_work);
    destroy_workqueue(simtemp_wq);
    misc_deregister(&simtemp_dev);
    pr_info("nxp_simtemp driver unloaded\n");
}

module_init(simtemp_init);
module_exit(simtemp_exit);

