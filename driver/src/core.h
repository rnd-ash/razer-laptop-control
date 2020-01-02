// SPDX-License-Identifier: GPL-2.0

#ifndef CORE_H_
#define CORE_H_

#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"
#include "defines.h"

// Struct to hold some basic data about the laptops current state
struct razer_laptop {
	int product_id;			// Product ID
	struct usb_device *usb_dev;	// USB Device we wish to talk to
	struct mutex lock;		// Mutex
	int fan_rpm;			// Fan RPM of manual mod (0 = AUTO)
	int gaming_mode;		// Gaming mode (0 = Balanced) (1 = Gaming AKA Higher CPU TDP)
};


struct razer_packet {
    __u8 __res;
    __u8 dev;
    __u8 __res2[3];
    __u8 args_size;
    __u8 cmd_id;
    __u8 sub_cmd_id;
    __u8 args[82]; // Includes CS bit and end 0x00
};

char *getDeviceDescription(int product_id);
void crc(char *buffer);
int send_payload(struct usb_device *usb_dev, char *buffer, unsigned long minWait, unsigned long maxWait);


#endif