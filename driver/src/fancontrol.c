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
    case BLADE_2020_ADV:
		return 1;
	default:
		return 0;
	}
}

int boost_mode_allowed(__u32 product_id)
{
    switch (product_id) {
    case BLADE_2020_ADV:
        return 1;
    default:
        return 0;
    }
}

void set_fan_rpm(unsigned long x, struct razer_laptop *laptop) {
    mutex_lock(&laptop->lock);
    if(laptop->power_mode < 4) // custom mode do not support fan profile
    {
        u8 request_fan_speed;
        char buffer[90] = {0x00};
        if (x != 0) {
            request_fan_speed = clamp_fan_rpm(x, laptop->product_id);
            laptop->fan_rpm = request_fan_speed * 100;
            // All packets
            buffer[0] = 0x00;
            buffer[1] = 0x1f;
            buffer[2] = 0x00;
            buffer[3] = 0x00;
            buffer[4] = 0x00;

            // Unknown
            buffer[5] = 0x04;
            buffer[6] = 0x0d;
            buffer[7] = 0x82;
            buffer[8] = 0x00;
            buffer[9] = 0x01;
            buffer[10] = 0x00;
            buffer[11] = 0x00;
            send_payload(laptop->usb_dev, buffer, 3400, 3800);

            // Unknown
            buffer[5] = 0x04;
            buffer[6] = 0x0d;
            buffer[7] = 0x02;
            buffer[8] = 0x00;
            buffer[9] = 0x01;
            buffer[10] = laptop->power_mode;
            buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
            send_payload(laptop->usb_dev, buffer, 204000, 205000);

            // Set fan RPM
            buffer[5] = 0x03;
            buffer[6] = 0x0d;
            buffer[7] = 0x01;
            buffer[8] = 0x00;
            buffer[9] = 0x01;
            buffer[10] = request_fan_speed;
            buffer[11] = 0x00;
            send_payload(laptop->usb_dev, buffer, 3400, 3800);

            // Unknown
            buffer[5] = 0x04;
            buffer[6] = 0x0d;
            buffer[7] = 0x82;
            buffer[8] = 0x00;
            buffer[9] = 0x02;
            buffer[10] = 0x00;
            buffer[11] = 0x00;
            send_payload(laptop->usb_dev, buffer, 3400, 3800);
        } else {
            laptop->fan_rpm = 0;
        }
        // Fan mode
        buffer[5] = 0x04;
        buffer[6] = 0x0d;
        buffer[7] = 0x02;
        buffer[8] = 0x00;
        buffer[9] = 0x02;
        buffer[10] = laptop->power_mode;
        buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
        send_payload(laptop->usb_dev, buffer, 204000, 205000);

        if (x != 0) {
            // Set fan RPM
            buffer[5] = 0x03;
            buffer[6] = 0x0d;
            buffer[7] = 0x01;
            buffer[8] = 0x00;
            buffer[9] = 0x02;
            buffer[10] = request_fan_speed;
            buffer[11] = 0x00;
            send_payload(laptop->usb_dev, buffer, 0, 0);
        }
    }
    mutex_unlock(&laptop->lock);
}

int set_power_mode(unsigned long x, struct razer_laptop *laptop) {
    mutex_lock(&laptop->lock);
    int res = 0;
    if (x >= 0 && x <= 2) {
        char buffer[90];
        // Device doesn't support creator mode
        if (x == 2 && !creator_mode_allowed(laptop->product_id)) {
            x = 1;
        }

        laptop->power_mode = x;
        memset(buffer, 0x00, sizeof(buffer));
        // All packets
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
        buffer[10] = laptop->power_mode;
        buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00; // Custom RPM?
        res = send_payload(laptop->usb_dev, buffer, 0, 0);
    }
    else if(x == 4)
    {
        laptop->power_mode = x;
    }
    mutex_unlock(&laptop->lock);

    return res;
}

int set_custom_power_mode(unsigned long cpu_boost, unsigned long gpu_boost, struct razer_laptop *laptop)
{
    mutex_lock(&laptop->lock);
    if(laptop->power_mode == 4)
    {
        char buffer[90];
        if(cpu_boost == 3 && !boost_mode_allowed(laptop->product_id))
        {
            cpu_boost = 2;
        }

        laptop->cpu_boost = cpu_boost;
        laptop->gpu_boost = gpu_boost;
        memset(buffer, 0x00, sizeof(buffer));

        // All packets
        buffer[0] = 0x00;
        buffer[1] = 0x1f;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;

        // Begin transaction
        buffer[5] = 0x04;
        buffer[6] = 0x0d;
        buffer[7] = 0x82;
        buffer[8] = 0x00;
        buffer[9] = 0x01;
        buffer[10] = 0x00;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // Set power mode custom
        buffer[5] = 0x04;
        buffer[6] = 0x0d;
        buffer[7] = 0x02;
        buffer[8] = 0x00;
        buffer[9] = 0x01;
        buffer[10] = laptop->power_mode;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // Clear cpu boost
        buffer[5] = 0x03;
        buffer[6] = 0x0d;
        buffer[7] = 0x87;
        buffer[8] = 0x00;
        buffer[9] = 0x01;
        buffer[10] = 0x00;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // Set cpu boost
        buffer[5] = 0x03;
        buffer[6] = 0x0d;
        buffer[7] = 0x07;
        buffer[8] = 0x00;
        buffer[9] = 0x01;
        buffer[10] = laptop->cpu_boost;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // Clear gpu boost
        buffer[5] = 0x03;
        buffer[6] = 0x0d;
        buffer[7] = 0x87;
        buffer[8] = 0x00;
        buffer[9] = 0x02;
        buffer[10] = 0x00;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // Set gpu boost
        buffer[5] = 0x03;
        buffer[6] = 0x0d;
        buffer[7] = 0x07;
        buffer[8] = 0x00;
        buffer[9] = 0x02;
        buffer[10] = laptop->gpu_boost;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // End transaction
        buffer[5] = 0x04;
        buffer[6] = 0x0d;
        buffer[7] = 0x82;
        buffer[8] = 0x00;
        buffer[9] = 0x02;
        buffer[10] = 0x00;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);

        // End set power mode
        buffer[5] = 0x04;
        buffer[6] = 0x0d;
        buffer[7] = 0x82;
        buffer[8] = 0x00;
        buffer[9] = 0x02;
        buffer[10] = laptop->power_mode;
        buffer[11] = 0x00;
        send_payload(laptop->usb_dev, buffer, 3400, 3800);
    }
    mutex_unlock(&laptop->lock);

    // always 0 since send_payload return only 0
    return 0;
}
