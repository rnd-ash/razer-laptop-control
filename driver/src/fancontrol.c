#include "fancontrol.h"



__u8 clampFanRPM(unsigned int rpm) {
    rpm = rpm / 100; // Divide by 100 cause thats how device wants it
    if (rpm > MAX_FAN_RPM) {
        return MAX_FAN_RPM;
    } else if (rpm < MIN_FAN_RPM) {
        return MIN_FAN_RPM;
    } else {
        return (__u8) rpm;
    }
}