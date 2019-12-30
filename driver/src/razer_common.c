// SPDX-License-Identifier: GPL-2.0
#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"
#include "defines.h"
#include "core.h"


MODULE_AUTHOR("Ashcon Mohseninia");
MODULE_DESCRIPTION("Razer system control driver for laptops");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

/**
 * Returns a pointer to string of the product name of the device
 */
static char *getDeviceDescription(int product_id)
{
	switch (product_id) {
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
struct key_colour {
	__u8 r;
	__u8 g;
	__u8 b;
};

struct key_row {
	__u8 __res;
	__u8 id;
	__u8 __res1[3];
	__u8 cmd_id;
	__u8 sub_cmd;
	__u8 sub_cmd_id;
	__u8 unk1;
	__u8 row_id;
	__u8 __res4;
	__u8 unk2;
	__u8 __res6[3];
	struct key_colour key_data[15];
};

// Struct to hold some basic data about the laptops current state
struct razer_laptop {
	int product_id;			// Product ID
	struct usb_device *usb_dev;	// USB Device we wish to talk to
	struct mutex lock;		// Mutex
	int fan_rpm;			// Fan RPM of manual mod (0 = AUTO)
	int gaming_mode;		// Gaming mode (0 = Balanced) (1 = Gaming AKA Higher CPU TDP)
};

static ssize_t wave_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	struct key_row rows[6];
	memset(rows, 0x00, sizeof(rows));
	unsigned char tmp[90];
	int i;
	for (i = 0; i < 6; i++) {
		memset(tmp, 0x00, sizeof(tmp));
		rows[i].id = 0x1f;
		rows[i].cmd_id = 0x34;
		rows[i].sub_cmd = 0x03;
		rows[i].sub_cmd_id = 0x0b;
		rows[i].row_id = i;
		rows[i].unk1 = 0xff;
		rows[i].unk2 = 0x0f;
		int x;
		for (x = 0; x <= 15; x++) {
			if (x % 3 == 0 && x != 0) {
				rows[i].key_data[x-1].r = 255/i;
				rows[i].key_data[x-2].g = 255/i;
				rows[i].key_data[x-3].b = 255/i;
			}
		}
		memcpy(tmp, &rows[i], 90);
		int t;
		dev_info(dev, "Sending\n");
		mutex_lock(&laptop->lock);
		send_payload(laptop->usb_dev, tmp, 1000, 1000);
		mutex_unlock(&laptop->lock);
		
	}
	char buffer2[90];
	mutex_lock(&laptop->lock);
	memset(buffer2, 0x00, sizeof(buffer2));
	buffer2[0] = 0x00;
	buffer2[1] = 0x1f;
	buffer2[2] = 0x00;
	buffer2[3] = 0x00;
	buffer2[4] = 0x00;
	buffer2[5] = 0x02;
	buffer2[6] = 0x03;
	buffer2[7] = 0x0a;
	buffer2[8] = 0x05;
	buffer2[9] = 0x00;
	send_payload(laptop->usb_dev, buffer2, 1000, 1000);
	mutex_unlock(&laptop->lock);
	return count;
}
static ssize_t spectrum_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	return count;
}
static ssize_t reactive_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	return count;
}
static ssize_t breath_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	return count;
}
static ssize_t static_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	return count;
}

/**
 * Called on reading fan_rpm sysfs entry
 */
static ssize_t fan_rpm_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);

	if (laptop->fan_rpm == 0)
		return sprintf(buf, "%s", "Automatic (0)\n");

	return sprintf(buf, "%d RPM\n", laptop->fan_rpm);
}

/**
 * Called on reading power_mode sysfs entry
 */
static ssize_t power_mode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);

	if (laptop->gaming_mode == 0)
		return sprintf(buf, "%s", "Balanced (0)\n");
	else if (laptop->gaming_mode == 1)
		return sprintf(buf, "%s", "Gaming (1)\n");

	return sprintf(buf, "%s", "Creator (2)\n");
}

static ssize_t fan_rpm_store(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	unsigned long x;
	__u8 request_fan_speed;
	char buffer[90];

	mutex_lock(&laptop->lock);

	memset(buffer, 0x00, sizeof(buffer));
	if (kstrtol(buf, 10, &x)) { // Convert users input to integer
		dev_warn(dev, "User entered an invalid input for fan rpm. Defaulting to auto");
		request_fan_speed = 0;
	}
	if (x != 0) {
		request_fan_speed = clampFanRPM(x, laptop->product_id);
		dev_info(dev, "Requesting MANUAL fan at %d RPM",
			 ((int) request_fan_speed * 100));
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
		send_payload(laptop->usb_dev, buffer, 3400, 3800);

		// Unknown
		buffer[5] = 0x04;
		buffer[6] = 0x0d;
		buffer[7] = 0x02;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = laptop->gaming_mode;
		buffer[11] = laptop->fan_rpm != 0 ? 0x01 : 0x00;
		send_payload(laptop->usb_dev, buffer, 204000, 205000);

		// Set fan RPM
		buffer[5] = 0x03;
		buffer[6] = 0x0d;
		buffer[7] = 0x01;
		buffer[8] = 0x00;
		buffer[9] = 0x01;
		buffer[10] = request_fan_speed;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer, 3400, 3800);

		// Unknown
		buffer[5] = 0x04;
		buffer[6] = 0x0d;
		buffer[7] = 0x82;
		buffer[8] = 0x00;
		buffer[9] = 0x02;
		buffer[10] = 0x00;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer, 3400, 3800);
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
	send_payload(laptop->usb_dev, buffer, 204000, 205000);

	if (x != 0) {
		// Set fan RPM
		buffer[5] = 0x03;
		buffer[6] = 0x0d;
		buffer[7] = 0x01;
		buffer[8] = 0x00;
		buffer[9] = 0x02;
		buffer[10] = request_fan_speed;
		buffer[11] = 0x00;
		send_payload(laptop->usb_dev, buffer, 0, 0);
	}
	mutex_unlock(&laptop->lock);
	return count;
}

/**
 * Sets gaming mode to on / off depending on user's input
 *
 * This is quite simple. Just send packet with command ID of 0x02. with the 12th
 * bit toggled depending on if gaming mode should be on or off.
 *
 * Cause ID 0x02 also deals with enabling / disabling manual fan RPM control, we
 * have to send the current fan control state as well within the message
 *
 */
static ssize_t power_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct razer_laptop *laptop = dev_get_drvdata(dev);
	ssize_t retval = count;
	unsigned long x;

	mutex_lock(&laptop->lock);

	if (kstrtol(buf, 10, &x)) {
		dev_warn(dev,
			 "User entered an invalid input for power mode. Defaulting to balanced");
		x = 0;
	}
	if (x == 1 || x == 0 || x == 2) {
		char buffer[90];

		if (x == 0) {
			dev_info(dev, "%s", "Enabling Balanced power mode");
		} else if (x == 2 &&
			   creatorModeAllowed(laptop->product_id) == 1) {
			dev_info(dev, "%s", "Enabling Gaming power mode");
		} else if (x == 1) {
			dev_info(dev, "%s", "Enabling Gaming power mode");
		} else {
			x = 1;
			dev_warn(dev, "%s",
				 "Creator mode not allowed, falling back to performance");
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
		send_payload(laptop->usb_dev, buffer, 0, 0);
	} else {
		retval = -EINVAL;
	}

	mutex_unlock(&laptop->lock);

	return retval;
}

// Set our device attributes in sysfs
static DEVICE_ATTR_RW(fan_rpm);
static DEVICE_ATTR_RW(power_mode);
static DEVICE_ATTR_WO(wave_mode);
static DEVICE_ATTR_WO(spectrum_mode);
static DEVICE_ATTR_WO(reactive_mode);
static DEVICE_ATTR_WO(breath_mode);
static DEVICE_ATTR_WO(static_mode);

// Called on load module
static int razer_laptop_probe(struct hid_device *hdev,
			      const struct hid_device_id *id)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct razer_laptop *dev;

	dev = kzalloc(sizeof(struct razer_laptop), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	mutex_init(&dev->lock);
	dev->usb_dev = usb_dev;
	dev->fan_rpm = 0;
	dev->gaming_mode = 0;
	dev->product_id = hdev->product;
	// Internal laptop touchpad found (over USB Bus).
	// Don't bind to it so unload.
	// Only the Keyboard can control the fan speed
	if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_KEYBOARD) {
		kfree(dev);
		return -ENODEV;
	}
	dev_info(&intf->dev, "Found supported device: %s\n",
		 getDeviceDescription(dev->product_id));
	device_create_file(&hdev->dev, &dev_attr_fan_rpm);
	device_create_file(&hdev->dev, &dev_attr_power_mode);
	device_create_file(&hdev->dev, &dev_attr_wave_mode);
	device_create_file(&hdev->dev, &dev_attr_static_mode);
	device_create_file(&hdev->dev, &dev_attr_reactive_mode);
	device_create_file(&hdev->dev, &dev_attr_spectrum_mode);
	device_create_file(&hdev->dev, &dev_attr_breath_mode);
	hid_set_drvdata(hdev, dev);
	if (hid_parse(hdev)) {
		hid_err(hdev, "Failed to parse device!\n");
		kfree(dev);
		return -ENODEV;
	}
	if (hid_hw_start(hdev, HID_CONNECT_DEFAULT)) {
		hid_err(hdev, "Failed to start device!\n");
		kfree(dev);
		return -ENODEV;
	}

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
	device_remove_file(&hdev->dev, &dev_attr_static_mode);
	device_remove_file(&hdev->dev, &dev_attr_reactive_mode);
	device_remove_file(&hdev->dev, &dev_attr_spectrum_mode);
	device_remove_file(&hdev->dev, &dev_attr_breath_mode);
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
