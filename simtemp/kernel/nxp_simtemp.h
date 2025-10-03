#ifndef _NXP_SIMTEMP_H_
#define _NXP_SIMTEMP_H_

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#define SIMTEMP_RING_SIZE 128

struct simtemp_config {
    u32 sampling_ms;
    s32 threshold_mC;
    u32 mode; // 0=normal,1=noisy,2=ramp
};

struct simtemp_sample_internal {
    struct simtemp_sample data;
};

struct simtemp_stats_internal {
    u64 updates;
    u64 alerts;
    u64 errors;
};

struct simtemp_device {
    struct simtemp_config config;
    struct simtemp_stats_internal stats;

    /* Ring buffer */
    struct simtemp_sample_internal ring[SIMTEMP_RING_SIZE];
    u32 head;
    u32 tail;

    /* Locks */
    spinlock_t ring_lock;
    struct mutex config_lock;

    /* Wait queue */
    wait_queue_head_t wq;

    /* Work/timer */
    struct hrtimer timer;
    struct work_struct work;

    /* Char dev */
    struct miscdevice miscdev;
};

#endif
