// SPDX-License-Identifier: GPL-2.0

#include "core.h"

/**
 * Sends payload to the EC controller
 *
 * @param usb_device EC Controller USB device struct
 * @param buffer Payload buffer
 * @param minWait Minimum time to wait in us after sending the payload
 * @param maxWait Maximum time to wait in us after sending the payload
 */
int send_payload(struct usb_device *usb_dev, char *buffer,
			unsigned long minWait, unsigned long maxWait)
{
	char *buf2;
	int len;

	crc(buffer); // Generate checksum for payload
	buf2 = kmemdup(buffer, sizeof(char[90]), GFP_KERNEL);
	len = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      0x09,
			      0x21,
			      0x0300,
			      0x0002,
			      buf2,
			      90,
			      USB_CTRL_SET_TIMEOUT);
	// Sleep for a specified number of us. If we send packets too quickly,
	// the EC will ignore them
	usleep_range(minWait, maxWait);
	kfree(buf2);
	return 0;
}

/**
 * Generates a checksum Bit and places it in the 89th byte in the buffer array
 * If this is invalid then the EC will ignore the incomming message
 */
void crc(char *buffer)
{
	int res = 0;
	int i;
	// Simple CRC. Iterate over all bits from 2-87 and XOR them together
	for (i = 2; i < 88; i++)
		res ^= buffer[i];

	buffer[88] = res; // Set the checksum bit
}