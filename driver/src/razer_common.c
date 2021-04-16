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
MODULE_VERSION("1.3.0");

static int loaded = 0;
static razer_laptop laptop = {0x00};


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
	int i;
	if (count != 270) {
		dev_err(dev, "RGB Map expects 270 bytes. Got %ld Bytes", count);
		return -EINVAL;
	}
	mutex_lock(&laptop.lock);
	for (i = 0; i <= 5; i++) {
		memcpy(&matrix[i].keys, &buf[i*45], 45);
		sendRowDataToProfile(laptop.usb_dev, i);
	}
	displayProfile(laptop.usb_dev, 0);
	mutex_unlock(&laptop.lock);
	return count;
}

/**
 * Returns the name of the device
 */
static ssize_t product_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", getDeviceDescription(laptop.product_id));
}

/**
 * Called on reading fan_rpm sysfs entry
 */
static ssize_t fan_rpm_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", laptop.fan_rpm);
}

/**
 * Called on reading power_mode sysfs entry
 */
static ssize_t power_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", laptop.power_mode);
}

static ssize_t fan_rpm_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long x;
	if (kstrtol(buf, 10, &x)) { // Convert users input to integer
        #ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for fan rpm.");
        #endif
		return -EINVAL;
	}
	set_fan_rpm(x, &laptop);
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
static ssize_t power_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long x;
	if (kstrtol(buf, 10, &x)) {
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for power mode. Defaulting to balanced");
		#endif
		return -EINVAL;
	}

    set_power_mode(x, &laptop);
    set_custom_power_mode(laptop.cpu_boost, laptop.gpu_boost, &laptop);

	return count;
}

static ssize_t cpu_boost_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", laptop.cpu_boost);
}


static ssize_t cpu_boost_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long x;
	if (kstrtol(buf, 10, &x)) {
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for power mode. Defaulting to balanced");
		#endif
		return -EINVAL;
	}

    set_custom_power_mode(x, laptop.gpu_boost, &laptop);

	return count;
}

static ssize_t gpu_boost_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", laptop.gpu_boost);
}


static ssize_t gpu_boost_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long x;
	if (kstrtol(buf, 10, &x)) {
		#ifdef DEBUG
		dev_warn(dev, "User entered an invalid input for power mode. Defaulting to balanced");
		#endif
		return -EINVAL;
	}

    set_custom_power_mode(laptop.cpu_boost, x, &laptop);

	return count;
}

// Set our device attributes in sysfs
static DEVICE_ATTR_RW(fan_rpm);
static DEVICE_ATTR_RW(power_mode);
static DEVICE_ATTR_RW(cpu_boost);
static DEVICE_ATTR_RW(gpu_boost);
static DEVICE_ATTR_WO(key_colour_map);
static DEVICE_ATTR_RO(product);

static int backlight_sysfs_set(struct led_classdev *led_cdev, enum led_brightness brightness) {
    return sendBrightness(laptop.usb_dev, (__u8) brightness);
}

static enum led_brightness backlight_sysfs_get(struct led_classdev *ledclass) {
    return getBrightness(laptop.usb_dev);
}

static struct led_classdev kbd_backlight = {
        .name = "razerlaptop::kbd_backlight",
        .max_brightness = 255,
        .flags = LED_BRIGHT_HW_CHANGED,
        .brightness_set_blocking = &backlight_sysfs_set,
        .brightness_get	= &backlight_sysfs_get,
};

// Called on module load
static int razer_laptop_probe(struct hid_device *hdev, const struct hid_device_id *id) {
    int i;
    int rc;
    struct usb_interface *intf;
    struct usb_device *usb_dev;
    intf = to_usb_interface(hdev->dev.parent);
    usb_dev = interface_to_usbdev(intf);

    dev_info(&intf->dev, "Loading module\n");
    if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_KEYBOARD) {
        // Found the mouse - unload!
        return -ENODEV;
    }
    dev_info(&intf->dev, "Found supported laptop: %s\n", getDeviceDescription(hdev->product));

    mutex_init(&laptop.lock);
    // When the driver first loads (At boot), we know these will be the default values:
    laptop.fan_rpm = 0; // Auto
    laptop.power_mode = 0; // Normal
    laptop.cpu_boost = 1; // equal to Normal
    laptop.gpu_boost = 1; // equal to Normal
    laptop.product_id = hdev->product; // Product id
    laptop.usb_dev = usb_dev;

    for (i = 0; i < 6; i++) { // Label all the keyboard rows
        laptop.kbd.rows[i].rowid = i;
    }

    // Create SYSFS entries
    device_create_file(&hdev->dev, &dev_attr_fan_rpm);
    device_create_file(&hdev->dev, &dev_attr_power_mode);
    device_create_file(&hdev->dev, &dev_attr_cpu_boost);
    device_create_file(&hdev->dev, &dev_attr_gpu_boost);
    device_create_file(&hdev->dev, &dev_attr_key_colour_map);
    device_create_file(&hdev->dev, &dev_attr_product);

    // Now init the backlight stuff - Only do it once!
    if (!loaded) {
        rc = led_classdev_register(&intf->dev, &kbd_backlight);
        if (rc < 0) {
            hid_err(hdev, "Failed to setup backlight!\n");
            return rc;
        }
    }
    loaded = 1;

    // Now set driver data
    hid_set_drvdata(hdev, &laptop);
    if (hid_parse(hdev)) {
        hid_err(hdev, "Failed to parse device!\n");
        return -ENODEV;
    }
    if (hid_hw_start(hdev, HID_CONNECT_DEFAULT)) {
        hid_err(hdev, "Failed to start device!\n");
        return -ENODEV;
    }
    return 0;
}

// Called on unload
static void razer_laptop_remove(struct hid_device *hdev) {

    device_remove_file(&hdev->dev, &dev_attr_fan_rpm);
    device_remove_file(&hdev->dev, &dev_attr_power_mode);
    device_remove_file(&hdev->dev, &dev_attr_cpu_boost);
    device_remove_file(&hdev->dev, &dev_attr_gpu_boost);
    device_remove_file(&hdev->dev, &dev_attr_key_colour_map);
    device_remove_file(&hdev->dev, &dev_attr_product);
    if (loaded) { // Ensure this only happens once!
        led_classdev_unregister(&kbd_backlight);
        loaded = 0;
    }
    hid_hw_stop(hdev);
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
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2020_ADV)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2021_ADV)},

	// Stealths
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_MID)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_END)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_STEALTH)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_STEALTH_GTX)},
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2020_STEALTH)},
	
	// Books
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BOOK_2020)},

	// Pro's
	{ HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2020_PRO_FHD)},
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
