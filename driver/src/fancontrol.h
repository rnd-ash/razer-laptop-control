#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>


#define MAX_FAN_RPM 0x35
#define MIN_FAN_RPM 0x00 // 0 RPM

#define FAN_RPM_ID  0x01 // ID to control fan RPM

__u8 clampFanRPM(unsigned int rpm);

