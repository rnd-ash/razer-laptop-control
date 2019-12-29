#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"
#include "defines.h"

MODULE_AUTHOR("Ashcon Mohseninia");
MODULE_DESCRIPTION("Razer system control driver for laptops");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

/**
 * Returns a pointer to string of the product name of the device
 */
char * getDeviceDescription(int product_id)
{
	switch(product_id) {
	case BLADE_2016_END:
		return "Blade 15 late 2016";
	case BLADE_2018_BASE:
		return "Blade 15 2018 Base";
	case BLADE_2018_ADV:
		return "Blade 15 2018 Advanced";
	case BLADE_2018_MERC:
		return "Blade 15 2018 Mercury Edition";
	case BLADE_2018_PRO_FHD:
		return "Blade pro 2018 FHD";
	case BLADE_2019_ADV:
		return "Blade 15 2019 Advanced";
	case BLADE_2019_MERC:
		return "Blade 15 2019 Mercury Edition";
	case BLADE_2019_STEALTH:
		return "Blade 2019 Stealth";
	case BLADE_PRO_2019:
		return "Blade pro 2019";
	case BLADE_2016_PRO:
		return "Blade pro 2016";
	case BLADE_2017_PRO:
		return "Blade peo 2017";
	case BLADE_2017_STEALTH_END:
		return "Blade Stealth late 2017";
	case BLADE_2017_STEALTH_MID:
		return "Blade Stealth mid 2017";
	case BLADE_QHD:
		return "Blade QHD";
	default:
		return "UNKNOWN";
	}
}

// Struct to hold some basic data about the laptops current state
struct razer_laptop {
	int product_id;			// Product ID
	struct usb_device *usb_dev;	// USB Device we wish to talk to
	struct mutex lock;		// Mutex
	int fan_rpm;			// Fan RPM of manual mod (0 = AUTO)
	int gaming_mode;		// Gaming mode (0 = Balanced) (1 = Gaming AKA Higher CPU TDP)
};

/**
 * Called on reading fan_rpm sysfs entry
 */
static ssize_t get_fan_rpm(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	if (laptop->fan_rpm == 0) {
		return sprintf(buf, "%s", "Automatic (0)\n");
	} else {
		return sprintf(buf, "%d RPM\n", laptop->fan_rpm);
	}
}

/**
 * Called on reading power_mode sysfs entry
 */
static ssize_t get_performance_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	if (laptop->gaming_mode == 0) {
		return sprintf(buf, "%s", "Balanced (0)\n");
	} else if (laptop->gaming_mode == 1){
		return sprintf(buf, "%s", "Gaming (1)\n");
	} else {
		return sprintf(buf, "%s", "Creator (2)\n");
	}
}

/**
 * Generates a checksum Bit and places it in the 89th byte in the buffer array
 * If this is invalid then the EC will ignore the incomming message
 */
void crc(char * buffer)
{
	int res = 0;
	int i;
	// Simple CRC. Iterate over all bits from 2-87 and XOR them together
	for (i = 2; i < 88; i++) {
		res ^= buffer[i];
	}
	buffer[88] = res; // Set the checksum bit
}

/**
 * Sends payload to the EC controller
 * 
 * @param usb_device EC Controller USB device struct
 * @param buffer Payload buffer
 * @param minWait Minimum time to wait in us after sending the payload
 * @param maxWait Maximum time to wait in us after sending the payload
 */
int send_payload(struct usb_device *usb_dev, char *buffer, unsigned long minWait, unsigned long maxWait)
{
	char * buf2;
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
	// Sleep for a specified number of us. If we send packets too quickly, the EC will ignore them
	usleep_range(minWait,maxWait);
	kfree(buf2);
	return 0;
}

static ssize_t set_fan_rpm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	unsigned long x;
	__u8 request_fan_speed;
	char buffer[90];
	memset(buffer, 0x00, sizeof(buffer));
	if (kstrtol(buf, 10, &x)) { // Convert users input to integer
		dev_warn(dev, "User entered an invalid input for fan rpm. Defaulting to auto");
		request_fan_speed = 0;
	}
	if (x != 0) {
		request_fan_speed = clampFanRPM(x, laptop->product_id);
		dev_info(dev, "Requesting MANUAL fan at %d RPM", ((int) request_fan_speed * 100));
		laptop->fan_rpm = request_fan_speed * 100;
		// All packets
		buffer[0] = 0x00;
		buffer[1] = 0x1f;
		buffer[2] = 0x00;
		buffer[3] = 0x00;
		buffer[4] = 0x00;

		// Unknown
		buffer[5] = 0x04;
		buffer[6] = 0x0d;
		buffer[7] = 0x82;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = 0x00;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer,3400,3800);

		// Unknown
		buffer[5] = 0x04;
		buffer[6] = 0x0d;
		buffer[7] = 0x02;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = laptop->gaming_mode;
		buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
		send_payload(laptop->usb_dev, buffer,204000,205000);

		// Set fan RPM
		buffer[5] = 0x03;
		buffer[6] = 0x0d;
		buffer[7] = 0x01;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = request_fan_speed;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer,3400,3800);

		// Unknown
		buffer[5] = 0x04;
		buffer[6] = 0x0d;
		buffer[7] = 0x82;
		buffer[8] = 0x00;
		buffer[9] = 0x02;
		buffer[10] = 0x00;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer,3400, 3800);
	} else {
		dev_info(dev, "Requesting AUTO Fan");
		laptop->fan_rpm = 0;
	}

	// Fan mode
	buffer[5] = 0x04;
	buffer[6] = 0x0d;
	buffer[7] = 0x02;
	buffer[8] = 0x00;
	buffer[9] = 0x02;
	buffer[10] = laptop->gaming_mode;
	buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
	send_payload(laptop->usb_dev, buffer,204000,205000);

	if (x != 0) {
		// Set fan RPM
		buffer[5] = 0x03;
		buffer[6] = 0x0d;
		buffer[7] = 0x01;
		buffer[8] = 0x00;
		buffer[9] = 0x02;
		buffer[10] = request_fan_speed;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer,0,0);
	}
	return count;
}

/**
 * Sets gaming mode to on / off depending on user's input
 * 
 * This is quite simple. Just send packet with command ID of 0x02. with the 12th bit toggled depending on if
 * gaming mode should be on or off.
 * 
 * Cause ID 0x02 also deals with enabling / disabling manual fan RPM control, we have to send the current
 * fan control state as well within the message
 * 
 */
static ssize_t set_performance_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	unsigned long x;
	if (kstrtol(buf, 10, &x)) {
		dev_warn(dev, "User entered an invalid input for power mode. Defaulting to balanced");
		x = 0;
	}
	if (x == 1 || x == 0 || x == 2){
		char buffer[90];

		if (x == 0) {
			dev_info(dev,"%s", "Enabling Balanced power mode");
		} else if (x == 2 && creatorModeAllowed(laptop->product_id) == 1) {
			dev_info(dev,"%s", "Enabling Gaming power mode");
		} else if (x == 1) {
			dev_info(dev,"%s", "Enabling Gaming power mode");
		} else {
			x = 1;
			dev_warn(dev,"%s", "Creator mode not allowed, falling back to performance");
		}
		laptop->gaming_mode = x;
		memset(buffer, 0x00, sizeof(buffer));
		// All packets
		buffer[0] = 0x00;
		buffer[1] = 0x1f;
		buffer[2] = 0x00;
		buffer[3] = 0x00;
		buffer[4] = 0x00;

		buffer[5] = 0x04;
		buffer[6] = 0x0d;
		buffer[7] = 0x02;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = laptop->gaming_mode;
		buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
		send_payload(laptop->usb_dev, buffer,0,0);
		return count;
	} else {
		return -EINVAL;
	}
	return count;
}

// Set our device attributes in sysfs
static DEVICE_ATTR(fan_rpm, 0664, get_fan_rpm, set_fan_rpm);
static DEVICE_ATTR(power_mode, 0664, get_performance_mode, set_performance_mode);

// Called on load module
static int razer_laptop_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct razer_laptop *dev = NULL;
	dev = kzalloc(sizeof(struct razer_laptop), GFP_KERNEL);

	if (dev == NULL) {
		dev_err(&intf->dev, "Out of memory!\n");
		return -ENOMEM;
	}
	mutex_init(&dev->lock);
	dev->usb_dev = usb_dev;
	dev->fan_rpm = 0;
	dev->gaming_mode = 0;
	dev->product_id = hdev->product;
	// Internal laptop touchpad found (over USB Bus). Don't bind to it so unload
	// Only the Keyboard can control the fan speed
	if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_KEYBOARD) {
		kfree(dev);
		return -ENODEV;
	}
	dev_info(&intf->dev, "Found supported device: %s\n", getDeviceDescription(dev->product_id));
	device_create_file(&hdev->dev, &dev_attr_fan_rpm);
	device_create_file(&hdev->dev, &dev_attr_power_mode);

	hid_set_drvdata(hdev, dev);
	if(hid_parse(hdev)) {
		hid_err(hdev, "Failed to parse device!\n");
		kfree(dev);
		return -ENODEV;
	}
	if(hid_hw_start(hdev, HID_CONNECT_DEFAULT)) {
		hid_err(hdev, "Failed to start device!\n");
		kfree(dev);
		return -ENODEV;
	}

	printk(KERN_INFO "Razer_laptop_control: Loaded\n");
	return 0;
}

// Called on unload module
static void razer_laptop_remove(struct hid_device *hdev)
{
	struct device *dev;
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	dev = hid_get_drvdata(hdev);
	device_remove_file(&hdev->dev, &dev_attr_fan_rpm);
	device_remove_file(&hdev->dev, &dev_attr_power_mode);
	hid_hw_stop(hdev);
	kfree(dev);
	dev_info(&intf->dev, "Razer_laptop_control: Unloaded\n");
}

// Support list for module
static const struct hid_device_id table[] = {
	// 15"
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2016_END)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2018_ADV)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2018_BASE)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2018_MERC)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_ADV)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_PRO_2019)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_MERC)},

	// Stealths
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_MID)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_END)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_STEALTH)},

	// Pro's
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2018_PRO_FHD)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_PRO)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_PRO)},

	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_QHD)},
	{ }
};
MODULE_DEVICE_TABLE(hid, table);
static struct hid_driver razer_sc_driver = {
	.name = "Razer laptop System control driver",
	.probe = razer_laptop_probe,
	.remove = razer_laptop_remove,
	.id_table = table,
};
module_hid_driver(razer_sc_driver);
