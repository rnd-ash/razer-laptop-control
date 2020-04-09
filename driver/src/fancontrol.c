// SPDX-License-Identifier: GPL-2.0
#include "fancontrol.h"

/*
* Returns the highest valid fan RPM for manual fan control
*
* We could through a higher value at the EC, it may even accept it which would
* be dangerous, so limit it to what we know is safe based on using Synapse.
*
* Each model of Blade seems to have a different upper limit, so we require the
* product ID to set the correct upper bound
*/
static int get_max_fan_rpm(__u32 product_id)
{
	switch (product_id) {
	case BLADE_2018_ADV:
	case BLADE_2018_BASE:
		return MAX_FAN_RPM_2018_BLADE;
	case BLADE_2019_ADV:
	case BLADE_2019_MERC:
	case BLADE_PRO_2019:
	case BLADE_2019_STEALTH:
		return MAX_FAN_RPM_STEALTH;
	default:
		return MAX_FAN_RPM_DEFAULT;
	}
}

u8 clamp_fan_rpm(unsigned int rpm, __u32 product_id)
{
	int max_allowed = get_max_fan_rpm(product_id);

	if (rpm > max_allowed)
		return (u8)(max_allowed / 100);
	else if (rpm < ABSOLUTE_MIN_FAN_RPM)
		return (u8)(ABSOLUTE_MIN_FAN_RPM / 100);

	return (u8)(rpm / 100);
}

int creator_mode_allowed(__u32 product_id)
{
	switch (product_id) {
	case BLADE_2019_ADV:
	case BLADE_2019_MERC:
		return 1;
	default:
		return 0;
	}
}

void setFanRPM(razer_laptop* laptop, int rpm) {
	mutex_lock(&laptop->lock);
	
	mutex_unlock(&laptop->lock);
}

void setPower(razer_laptop* laptop, __u8 power) {
	mutex_lock(&laptop->lock);

	int power_mode = power;
	// Fallback if creator is not allowed
	if (power_mode == CREATOR && !creator_mode_allowed(laptop->product_id)) {
		power_mode = GAMING;
	}
	hid_err(laptop->usb_dev, "Setting power mode to %d", power);
	char buffer[90];
	memset(buffer, 0x00, sizeof(buffer));
	buffer[0] = 0x00;
	buffer[1] = 0x1f;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	buffer[4] = 0x00;

	buffer[5] = 0x04;
	buffer[6] = 0x0d;
	buffer[7] = 0x02;
	buffer[8] = 0x00;
	buffer[9] = 0x01;
	buffer[10] = power_mode;
	buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
	send_payload(laptop->usb_dev, buffer, 0, 0);
	laptop->power = power_mode;
	mutex_unlock(&laptop->lock);
}

int getFanRPM(razer_laptop* laptop) {
	return laptop->fan_rpm;
}

PERF_MODE getPower(razer_laptop* laptop) {
	return NORMAL;
}
