#ifndef _NXP_SIMTEMP_H
#define _NXP_SIMTEMP_H

#include <linux/types.h>

// Estructura compartida kernel <-> user
struct simtemp_sample {
    __u64 timestamp_ns;  // tiempo de muestra
    __s32 temp_mC;       // temperatura en milicelsius
    __u32 flags;         // bit0 = NEW_SAMPLE, bit1 = THRESHOLD_CROSSED
};

#endif /* _NXP_SIMTEMP_H */

