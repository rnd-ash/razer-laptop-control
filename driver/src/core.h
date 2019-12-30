// SPDX-License-Identifier: GPL-2.0
#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"
#include "defines.h"

void crc(char *buffer);
int send_payload(struct usb_device *usb_dev, char *buffer, unsigned long minWait, unsigned long maxWait);