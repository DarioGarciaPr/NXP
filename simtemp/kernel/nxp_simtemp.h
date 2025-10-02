// nxp_simtemp_ioctl.h
#ifndef NXP_SIMTEMP_IOCTL_H
#define NXP_SIMTEMP_IOCTL_H

#include <linux/ioctl.h>

// Caracter único para nuestro dispositivo
#define SIMTEMP_MAGIC 't'

// IOCTLs
#define SIMTEMP_SET_THRESHOLD _IOW(SIMTEMP_MAGIC, 1, int)   // Configura el threshold en m°C
#define SIMTEMP_SET_SAMPLING  _IOW(SIMTEMP_MAGIC, 2, int)   // Configura el sampling en ms

#endif // NXP_SIMTEMP_IOCTL_H

