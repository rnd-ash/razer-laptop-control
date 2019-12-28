#include <linux/kernel.h>
#include "defines.h"

#define MAX_FAN_RPM_DEFAULT 5000 // Highest possible fan RPM by default (not laptop specific)
#define ABSOLUTE_MIN_FAN_RPM 3500 // Lowest possible fan RPM in manual Mode (ALL LAPTOPS)


#define MAX_FAN_RPM_2018_BLADE 5000 // 
#define MAX_FAN_RPM_STEALTH 5300 // 2017 Mid / Late Stealths

#define FAN_RPM_ID  0x01 // ID to control fan RPM

/**
 * Returns the highest valid fan RPM for manual fan control
 * 
 * We could through a higher value at the EC, it may even accept it which would be dangerous,
 * so limit it to what we know is safe based on using Synapse.
 * 
 * Each model of Blade seems to have a different upper limit, so we require the product ID 
 * to set the correct upper bound
 */
int get_max_fan_rpm(__u32 product_id);

/**
 * Clamps input manual fan RPM to what is allowed by the laptop
 */
__u8 clampFanRPM(unsigned int rpm, __u32 product_id);

