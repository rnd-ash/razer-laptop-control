// SPDX-License-Identifier: GPL-2.0
#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"
#include "defines.h"
#include "core.h"
#include "chroma.h"


MODULE_AUTHOR("Ashcon Mohseninia");
MODULE_DESCRIPTION("Razer system control driver for laptops");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.4");

/**
 * Function to send RGB data to keyboard to display
 * The keyboard is designed as a matrix with 6 rows (below is outline of my UK keyboard):
 * 
 *  Row 0: ESC - DEL 
 * 	Row 1: ` - BACKSPACE
 *  Row 2: TAB - ENTER
 *  Row 3: CAPS - #
 *  Row 4: SHIFT - SHIFT
 *  Row 5: CTRL - FN
 * 
 * This function takes RGB data and sends it to each row in the keyboard.
 * We expect 360 bytes (4 bytes per key), send in order row 0, key 0 to row 5, key 14.
 */
static ssize_t key_colour_map_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	struct razer_laptop *laptop;
	int i;
	laptop = dev_get_drvdata(dev);
	if (count != 270) {
		dev_err(dev, "RGB Map expects 270 bytes. Got %ld Bytes", count);
		return -EINVAL;
	}
	mutex_lock(&laptop->lock);
	for (i = 0; i <= 5; i++) {
		memcpy(&matrix[i].keys, &buf[i*45], 45);
		sendRowDataToProfile(laptop->usb_dev, i);
	}
	displayProfile(laptop->usb_dev, 0);
	mutex_unlock(&laptop->lock);
	return count;
}

static ssize_t brightness_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	struct razer_laptop *laptop;
	unsigned long brightness;
	laptop = dev_get_drvdata(dev);
	if (kstrtol(buf, 10, &brightness)) { // Convert users input to integer
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for brightness");
		#endif
		return -EINVAL;
	}

	if (brightness > 255 || brightness < 0) {
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for brightness");
		#endif
		return -EINVAL;
	}

	mutex_lock(&laptop->lock);
	sendBrightness(laptop->usb_dev, (__u8) brightness);
	mutex_unlock(&laptop->lock);
	return count;
}

static ssize_t brightness_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	int i;
	struct razer_laptop *laptop;
	char req[90];
	char resp[90];
	laptop = dev_get_drvdata(dev);
	#ifdef DEBUG
	dev_warn(dev, "Reading brightness");
	#endif
	memset(resp, 0x00, sizeof(resp));
	memset(req, 0x00, sizeof(req));
	req[1] = 0x1f;
	req[5] = 0x02;
	req[6] = 0x0E;
	req[7] = 0x84;
	req[8] = 0x01;
	recv_payload(laptop->usb_dev, req, resp, 800, 1000);

	for (i = 0; i < 20; i++) {
		dev_warn(dev, "%02X", resp[i]);
	}

	return sprintf(buf, "%d\n", (__u8)resp[9]);
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
	u8 request_fan_speed;
	char buffer[90];

	mutex_lock(&laptop->lock);

	memset(buffer, 0x00, sizeof(buffer));
	if (kstrtol(buf, 10, &x)) { // Convert users input to integer
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for fan rpm. Defaulting to auto");
		#endif
		request_fan_speed = 0;
	}
	if (x != 0) {
		request_fan_speed = clamp_fan_rpm(x, laptop->product_id);
		#ifdef DEBUG
		dev_info(dev, "Requesting MANUAL fan at %d RPM", ((int) request_fan_speed * 100));
		#endif
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
		#ifdef DEBUG
		dev_info(dev, "Requesting AUTO Fan");
		#endif
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
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for power mode. Defaulting to balanced");
		#endif
		return -EINVAL;
	}
	if (x == 1 || x == 0 || x == 2) {
		char buffer[90];

		#ifdef DEBUG
		if (x == 0) {
			dev_info(dev, "%s", "Enabling Balanced power mode");
		} else if (x == 2 && creator_mode_allowed(laptop->product_id) == 1) {
			dev_info(dev, "%s", "Enabling Gaming power mode");
		} else if (x == 1) {
			dev_info(dev, "%s", "Enabling Gaming power mode");
		} else {
			x = 1;
			#ifdef DEBUG
			dev_warn(dev, "%s", "Creator mode not allowed, falling back to performance");
			#endif
			x = 1;
		}
		#else
		if (x == 2 && creator_mode_allowed(laptop->product_id) == 0) {
			x = 1;
		}
		#endif
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
static DEVICE_ATTR_WO(key_colour_map);
static DEVICE_ATTR_RW(brightness);

// Called on load module
static int razer_laptop_probe(struct hid_device *hdev,
			      const struct hid_device_id *id)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct razer_laptop *dev;
	int c;
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
	dev_info(&intf->dev, "Found supported device: %s\n", getDeviceDescription(dev->product_id));
	device_create_file(&hdev->dev, &dev_attr_fan_rpm);
	device_create_file(&hdev->dev, &dev_attr_power_mode);
	device_create_file(&hdev->dev, &dev_attr_key_colour_map);
	device_create_file(&hdev->dev, &dev_attr_brightness);
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
	dev_info(&intf->dev, "Found supported device: %s\n", getDeviceDescription(dev->product_id));
	for (c=0; c <=5; c++) {
		memset(matrix[c].keys, 0xFF, sizeof(matrix[c].keys));	
	}
	displayMatrix(usb_dev);
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
	device_remove_file(&hdev->dev, &dev_attr_key_colour_map);
	device_remove_file(&hdev->dev, &dev_attr_brightness);
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
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_BASE)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_ADV)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_PRO_2019)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_MERC)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2020_BASE)},

	// Stealths
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_MID)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_END)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_STEALTH)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_STEALTH_GTX)},

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
