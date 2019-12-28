#include "fancontrol.h"

int get_max_fan_rpm(__u32 product_id) {
    switch (product_id)
    {
        case BLADE_2018_ADV:
        case BLADE_2018_BASE:
            return MAX_FAN_RPM_2018_BLADE;
        case BLADE_2017_STEALTH_MID:
        case BLADE_2017_STEALTH_END:
        case BLADE_2019_STEALTH:
            return MAX_FAN_RPM_STEALTH;
        default:
            return MAX_FAN_RPM_DEFAULT;
    }
}

__u8 clampFanRPM(unsigned int rpm, __u32 product_id) {
    int max_allowed = get_max_fan_rpm(product_id);
    if (rpm > max_allowed) {
        return (__u8) (max_allowed / 100);
    } else if (rpm < ABSOLUTE_MIN_FAN_RPM) {
        return (__u8) (ABSOLUTE_MIN_FAN_RPM / 100);
    }
    return (__u8) (rpm / 100);
}