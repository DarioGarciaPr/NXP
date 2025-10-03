#ifndef _NXP_SIMTEMP_IOCTL_H_
#define _NXP_SIMTEMP_IOCTL_H_

#include <linux/ioctl.h>

/*
 * IOCTL definitions for the NXP simulated temperature sensor
 *
 * User space applications can control the driver using these commands:
 *
 * - NXPSIM_IOCTL_SET_THRESHOLD:
 *      Set the threshold in milliCelsius.
 *
 * - NXPSIM_IOCTL_SET_SAMPLING_MS:
 *      Set the sampling interval in milliseconds.
 */

#define NXPSIM_IOCTL_MAGIC 'T'

/* Set temperature threshold (in milliCelsius) */
#define NXPSIM_IOCTL_SET_THRESHOLD   _IOW(NXPSIM_IOCTL_MAGIC, 1, int)

/* Set sampling interval (in milliseconds) */
#define NXPSIM_IOCTL_SET_SAMPLING_MS _IOW(NXPSIM_IOCTL_MAGIC, 2, int)

#endif /* _NXP_SIMTEMP_IOCTL_H_ */


