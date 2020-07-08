// SPDX-License-Identifier: GPL-2.0

#ifndef CHROMA_H_
#define CHROMA_H_

#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"
#include "core.h"


/**
 * Holds data regarding an individual key's colour and saturation
 */
struct key_colour { // I'm British. colour is spelt like this and not 'color' :)
    __u8 red; // Red level (0-255)
    __u8 green; // Green level (0-255)
    __u8 blue; // Blue level (0-255)
};

/**
 * Holds packet info to be sent to the device
 */
struct row_data {
    int row_id;
    int profileNumber;
    struct key_colour keys[15]; // White default colour
};

/**
 * Takes a char array and turns it into a char[90] packet to be sent to the keyboard
 * @param row_num Row Number (0 = F0-12 row, 5=CTRL+Fn+Win row)
 * @param profileNum Profile number to store row data in EEPROM
 * @param buffer;
 * @param usb USB device of keyboard
 */
int sendRowDataToProfile(struct usb_device *usb, int row_number);

/**
 * Tells the keyboard to display whatever data is stored for a given profile number
 * @param usb USB Device of keyboard
 * @param profileNum profile number to display
 */
int displayProfile(struct usb_device *usb, int profileNum);

/**
 * Sets the keyboard to the content of [matrix]
 */
int displayMatrix(struct usb_device *usb);

int sendBrightness(struct usb_device *usb, __u8 brightness);

int getBrightness(struct usb_device *usb);

extern struct row_data matrix[5];

#endif

