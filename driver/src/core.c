// SPDX-License-Identifier: GPL-2.0
#include "core.h"


/**
 * Returns a pointer to string of the product name of the device
 */
char *getDeviceDescription(int product_id)
{
	switch (product_id) {
	    // 15" laptops
	    case BLADE_2016_END:
	        return "Blade 15\" 2016";
	    case BLADE_2018_ADV:
	        return "Blade 2018 15\" advanced";
	    case BLADE_2018_BASE:
	        return "Blade 2018 15\" base";
	    case BLADE_2018_MERC:
	        return "Blade 2018 15\" Mercury edition";
	    case BLADE_2019_BASE:
	        return "Blade 2019 15\" base";
	    case BLADE_2019_ADV:
            return "Blade 2019 15\" advanced";
	    case BLADE_2019_MERC:
	        return "Blade 2019 15\" Mercury edition";
	    case BLADE_2020_BASE:
	        return "Blade 2020 15\" base";
	    case BLADE_2020_ADV:
	        return "Blade 2020 15\" advanced";

        // Stealth's
	    case BLADE_2017_STEALTH_MID:
	        return "Blade 2017 stealth";
	    case BLADE_2017_STEALTH_END:
	        return "late Blade 2017 stealth";
	    case BLADE_2019_STEALTH:
	        return "Blade 2019 stealth";
	    case BLADE_2019_STEALTH_GTX:
	        return "Blade 2019 stealth (With GTX)";

    // Pro laptops laptops
	    case BLADE_PRO_2019:
	        return "Blade 2019 pro";
	    case BLADE_2018_PRO_FHD:
	        return "Blade 2018 pro FHD";
	    case BLADE_2017_PRO:
	        return "Blade 2017 pro";
	    case BLADE_2016_PRO:
	        return "Blade 2016 pro";
    // Other?
	    case BLADE_QHD:
	        return "Blade QHD";
	default:
		return "UNKNOWN";
	}
}

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
 * Sends packet in req_buffer and puts device response
 * int resp_buffer.
 * 
 * TODO - Work out device calls for all querys (Fan RPM, Matrix brightness and Power mode).
 * Should be possible tracing packets when Synapse first loads up.
 */
int recv_payload(struct usb_device *usb_dev, char *req_buffer, char* resp_buffer, 
	unsigned long minWait, unsigned long maxWait) 
{
	uint request = HID_REQ_GET_REPORT;
	uint request_type = 0xA1;
	uint len;
	char *buf;
	buf = kzalloc(sizeof(char[90]), GFP_KERNEL);
	if (buf == NULL) {
		return -ENOMEM;
	}
	send_payload(usb_dev, req_buffer, minWait, maxWait);
	len = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
			request,
			request_type,
			0x300,
			0x0002,
			buf,
			90,
			USB_CTRL_SET_TIMEOUT);
	memcpy(resp_buffer, buf, 90);
	kfree(buf);

	if (len != 90) {
		dev_warn(&usb_dev->dev, "Razer laptop control: USB Response invalid. Got %d bytes. Expected 90.", len);
	}
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
