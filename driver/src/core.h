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

// Report Responses
#define RAZER_CMD_BUSY          0x01
#define RAZER_CMD_SUCCESSFUL    0x02
#define RAZER_CMD_FAILURE       0x03
#define RAZER_CMD_TIMEOUT       0x04
#define RAZER_CMD_NOT_SUPPORTED 0x05

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
    __u8 power_mode; // Power mode (0 = normal, 1 = gaming, 2 = creator, 4 = custom)
    __u8 cpu_boost; // only for custom mode
    __u8 gpu_boost; // only for custom mode
    keyboard_info kbd; // Keyboard data
} razer_laptop;

union transaction_id_union {
    unsigned char id;
    struct transaction_parts {
        unsigned char device : 3;
        unsigned char id : 5;
    } parts;
};

union command_id_union {
    unsigned char id;
    struct command_id_parts {
        unsigned char direction : 1;
        unsigned char id : 7;
    } parts;
};

 /* Status:
 * 0x00 New Command
 * 0x01 Command Busy
 * 0x02 Command Successful
 * 0x03 Command Failure
 * 0x04 Command No Response / Command Timeout
 * 0x05 Command Not Support
 *
 * Transaction ID used to group request-response, device useful when multiple devices are on one usb
 * Remaining Packets is the number of remaining packets in the sequence
 * Protocol Type is always 0x00
 * Data Size is the size of payload, cannot be greater than 80. 90 = header (8B) + data + CRC (1B) + Reserved (1B)
 * Command Class is the type of command being issued
 * Command ID is the type of command being send. Direction 0 is Host->Device, Direction 1 is Device->Host. AKA Get LED 0x80, Set LED 0x00
 *
 * */

struct razer_packet {
    __u8 status;
    union transaction_id_union transaction_id;
    __u16 remaining_packets;
    __u8 protocol_type; // 0x00
    __u8 data_size;
    __u8 command_class;
    union command_id_union command_id;
    __u8 args[80];
    __u8 crc;
    __u8 reserved; // 0x00
};

char *getDeviceDescription(int product_id);
__u8 crc(struct razer_packet *buffer);
int send_control_message(struct usb_device *usb_dev, void const *buffer, unsigned long minWait, unsigned long maxWait);
int get_usb_responce(struct usb_device *usb_dev, struct razer_packet* req_buffer, struct razer_packet* resp_buffer, unsigned long minWait, unsigned long maxWait);
void print_erroneous_report(struct razer_packet* report, char* driver_name, char* message);
struct razer_packet get_razer_report(unsigned char command_class, unsigned char command_id, unsigned char data_size);
struct razer_packet send_payload(struct usb_device *usb_dev, struct razer_packet *request_report);

void set_fan_rpm(unsigned long x, struct razer_laptop *laptop);
int set_power_mode(unsigned long x, struct razer_laptop *laptop);
int set_custom_power_mode(unsigned long cpu_boost, unsigned long gpu_boost, struct razer_laptop *laptop);

#endif
