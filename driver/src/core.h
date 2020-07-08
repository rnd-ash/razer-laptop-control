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
// Driver data representation
typedef struct {
    __u8 r; // Red channel
    __u8 g; // Green channel
    __u8 b; // Blue channel
} key;

typedef struct {
    __u8 rowid; // Row ID (0-5) used by the EC to update each row
    key keys[15]; // 15 keys per row (regardless if the LEDs are present or not)
} keyboard_row;

// Keyboard structs
typedef struct {
    __u8 brightness; // 0-255 brightness
    keyboard_row rows[6]; // 6 rows
} keyboard_info;

// Power/fan control struct
typedef struct razer_laptop {
    int product_id; // Product ID - Used for working out device capabilities
    struct mutex lock; // Lock mutex
    struct usb_device *usb_dev;	// USB Device for communication
    __u16 fan_rpm; // Fan RPM Set by driver (0 if it is auto!)
    __u8 power_mode; // Power mode (0 = normal, 1 = gaming, 2 = creator)
    keyboard_info kbd; // Keyboard data
} razer_laptop;


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
int recv_payload(struct usb_device *usb_dev, char *req_buffer, char* resp_buffer, unsigned long minWait, unsigned long maxWait);

#endif