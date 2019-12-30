/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/kernel.h>
#include "defines.h"

// Highest possible fan RPM by default (not laptop specific)
#define MAX_FAN_RPM_DEFAULT	5000

// Lowest possible fan RPM in manual Mode (ALL LAPTOPS)
#define ABSOLUTE_MIN_FAN_RPM	3500

//
#define MAX_FAN_RPM_2018_BLADE	5000

// 2017 Mid / Late Stealths
#define MAX_FAN_RPM_STEALTH	5300

#define FAN_RPM_ID  0x01 // ID to control fan RPM

/**
 * Clamps input manual fan RPM to what is allowed by the laptop
 */
u8 clamp_fan_rpm(unsigned int rpm, __u32 product_id);


/**
 * Some laptops are allowed an additional mode called 'creator'
 * which raises just GPU power
 */
int creatorModeAllowed(__u32 product_id);

