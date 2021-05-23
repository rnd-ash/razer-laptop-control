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
        case BLADE_2021_BASE:
            return "Blade 2021 15\" base";
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
	    case BLADE_2020_STEALTH:
		return "Blade 2020 stealth";

	// Book's
	    case BOOK_2020:
		return "Book 2020";

    // Pro laptops laptops
	    case BLADE_2020_PRO_FHD:
	        return "Blade 2020 pro FHD";
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
int send_control_message(struct usb_device *usb_dev, void const *buffer, unsigned long minWait, unsigned long maxWait)
{
    uint request = HID_REQ_SET_REPORT; // 0x09
    uint request_type = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT; // 0x21
    uint value = 0x300;
    uint report_index = 0x02;
    uint size = RAZER_USB_REPORT_LEN;
	char *buf;
	int len;

	/* crc(buffer); // Generate checksum for payload */
	buf = kmemdup(buffer, size, GFP_KERNEL);

    if(buf == NULL)
    {
        return -ENOMEM;
    }

	len = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			      request,
			      request_type,
			      value,
			      report_index,
			      buf,
			      size,
			      USB_CTRL_SET_TIMEOUT);
	// Sleep for a specified number of us. If we send packets too quickly,
	// the EC will ignore them
	usleep_range(minWait, maxWait);

	kfree(buf);
    if(len != size)
    {
        printk(KERN_WARNING "Razer laptop control: Device data transfer failed.");
    }

	return ((len < 0) ? len : ((len != size) ? -EIO : 0));
}

/**
 * Sends packet in req_buffer and puts device response
 * int resp_buffer.
 *
 * TODO - Work out device calls for all querys (Fan RPM, Matrix brightness and Power mode).
 * Should be possible tracing packets when Synapse first loads up.
 */
int get_usb_responce(struct usb_device *usb_dev, struct razer_packet* req_buffer, struct razer_packet* resp_buffer, unsigned long minWait, unsigned long maxWait)
{
	uint request = HID_REQ_GET_REPORT; // 0x01
	uint request_type = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN; // 0xA1
    uint value = 0x300;
    uint response_index = 0x02;
    uint size = RAZER_USB_REPORT_LEN;
	uint len;
    int result = 0;
	char *buf;

	buf = kzalloc(sizeof(char[90]), GFP_KERNEL);
	if (buf == NULL) {
		return -ENOMEM;
	}

    // Send request to device
	send_control_message(usb_dev, req_buffer, minWait, maxWait);
	len = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
			request,
			request_type,
			value,
			response_index,
			buf,
			size,
			USB_CTRL_SET_TIMEOUT);

	memcpy(resp_buffer, buf, RAZER_USB_REPORT_LEN);
	kfree(buf);

	if (len != 90) {
		dev_warn(&usb_dev->dev, "Razer laptop control: USB Response invalid. Got %d bytes. Expected 90.", len);
        result = 1;
	}

	return result;
}

struct razer_packet send_payload(struct usb_device *usb_dev, struct razer_packet *request_report)
{
    int retval = -1;
    struct razer_packet response_report = {0};

    request_report->crc = crc(request_report);

    retval = get_usb_responce(usb_dev, request_report, &response_report, 600, 800); //min max as parameters ? in openrazer ther are not

    if(retval == 0) {
        // Check the packet number, class and command are the same
        if(response_report.remaining_packets != request_report->remaining_packets ||
           response_report.command_class != request_report->command_class ||
           response_report.command_id.id != request_report->command_id.id) {
           // This can occur if we wire too many commands at once. Ignore
	   //print_erroneous_report(&response_report, "Razer laptop control", "Response doesn't match request");
//		} else if (response_report.status == RAZER_CMD_BUSY) {
//			print_erroneous_report(&response_report, "razerkbd", "Device is busy");
        } else if (response_report.status == RAZER_CMD_FAILURE) {
            print_erroneous_report(&response_report, "Razer laptop control", "Command failed");
        } else if (response_report.status == RAZER_CMD_NOT_SUPPORTED) {
            print_erroneous_report(&response_report, "Razer laptop control", "Command not supported");
        } else if (response_report.status == RAZER_CMD_TIMEOUT) {
            print_erroneous_report(&response_report, "Razer laptop control", "Command timed out");
        }
    } else {
        print_erroneous_report(&response_report, "Razer laptop control", "Invalid Report Length");
    }

    return response_report;
}

/**
 * Print report to syslog
 */
void print_erroneous_report(struct razer_packet* report, char* driver_name, char* message)
{
    printk(KERN_WARNING "%s: %s. Start Marker: %02x id: %02x Num Params: %02x Reserved: %02x Command: %02x Params: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x .\n",
           driver_name,
           message,
           report->status,
           report->transaction_id.id,
           report->data_size,
           report->command_class,
           report->command_id.id,
           report->args[0],  report->args[1],  report->args[2],  report->args[3], report->args[4],  report->args[5],
           report->args[6],  report->args[7],  report->args[8],  report->args[9], report->args[10], report->args[11],
           report->args[12], report->args[13], report->args[14], report->args[15]);
}

/**
 * Get initialised razer report
 */
struct razer_packet get_razer_report(unsigned char command_class, unsigned char command_id, unsigned char data_size)
{
    struct razer_packet new_report = {0};
    memset(&new_report, 0, sizeof(struct razer_packet));

    new_report.status = 0x00;
    new_report.transaction_id.id = 0x1F;
    new_report.remaining_packets = 0x00;
    new_report.protocol_type = 0x00;
    new_report.command_class = command_class;
    new_report.command_id.id = command_id;
    new_report.data_size = data_size;

    return new_report;
}

/**
 * Calculate the checksum for the usb message
 *
 * Checksum byte is stored in the 2nd last byte in the messages payload.
 * The checksum is generated by XORing all the bytes in the report starting
 * at byte number 2 (0 based) and ending at byte 88.
 */__u8 crc(struct razer_packet *buffer)
{
    __u8 res = 0;
    __u8 *_report = (__u8*) buffer;
	int i;
	// Simple CRC. Iterate over all bits from 2-87 and XOR them together
	for (i = 2; i < 88; i++)
		res ^= _report[i];

    return res;
}
