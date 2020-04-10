// SPDX-License-Identifier: GPL-2.0
#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "chroma.h"
#include "fancontrol.h"


static struct razer_laptop laptop;

MODULE_AUTHOR("Ashcon Mohseninia");
MODULE_DESCRIPTION("Razer laptop driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1.0");

// SYSFS Folders (for device)

static ssize_t power_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	setPower(&laptop, 1);
	return count;
}

static ssize_t fan_rpm_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	return count;
}

static ssize_t fan_rpm_show(struct device *dev, struct device_attribute *attr, char *buf) {
	switch(laptop.fan_rpm) {
		case 0:
			return sprintf(buf, "Fan auto\n");
		default:
			return sprintf(buf, "Fan %d rpm\n", laptop.fan_rpm);
	}
}

static ssize_t power_mode_show(struct device *dev, struct device_attribute *attr, char *buf) {
	switch(laptop.power) {
		case NORMAL:
			return sprintf(buf, "Power mode: Normal\n");
		case GAMING:
			return sprintf(buf, "Power mode: Gaming\n");
		case CREATOR:
			return sprintf(buf, "Power mode: Creator\n");
		default:
			return sprintf(buf, "Power mode: Unknown\n");
	}
}

static DEVICE_ATTR_RW(fan_rpm);
static DEVICE_ATTR_RW(power_mode);


// Called upon Daemon reading from procfs
static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
	hid_err(laptop.usb_dev, "Test");
	hid_err(laptop.usb_dev, "%s", file->f_path);
	return 0;
}

// Called upon Daemon writing to procfs
static ssize_t proc_write(struct file *file, const char __user *ubuf,size_t count, loff_t *ppos) {
	return -1;
}

// For proc_fs
static struct proc_dir_entry *procfolder;
static struct proc_dir_entry *procDaemon;

static const struct file_operations proc_fops = {
    .owner = THIS_MODULE,
    .read = proc_read,
    .write = proc_write,
};


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


static int backlight_sysfs_set(struct led_classdev *led_cdev, enum led_brightness brightness) {
	return setBrightness(laptop.usb_dev , (__u8) brightness);
}

static enum led_brightness backlight_sysfs_get(struct led_classdev *ledclass) {
	return getBrightness(laptop.usb_dev);
}  

// Keyboard backlight native control
static struct led_classdev kbd_backlight = {
	.name = "razerlaptop::kbd_backlight",
	.max_brightness = 255,
	.flags = LED_BRIGHT_HW_CHANGED,
	.brightness_set_blocking = &backlight_sysfs_set,
	.brightness_get	= &backlight_sysfs_get
};

// Called on load module
static int razer_laptop_probe(struct hid_device *hdev,
			      const struct hid_device_id *id)
{
	struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
	struct usb_device *usb_dev = interface_to_usbdev(intf);
	struct razer_laptop *dev;
	int c;
	dev = kzalloc(sizeof(struct razer_laptop), GFP_KERNEL);
	if (!dev) {
		return -ENOMEM;
	}

	/*	Razer are strange when it comes to laptop keyboards
		The keyboard has a controller that has 3 USB devices, that all have different
		names but all point to the same controller, and all 3 can accept the same USB commands.

		So to avoid useless loading on all the devices (unnecessary), check if the proc folder has
		been created, if it has, then one of the devices has already loaded the driver,
		so don't load on the new devices
	*/
	if (procfolder != NULL) {
		#ifdef DEBUG
		hid_err(hdev, "Not allowing secondary USB controller to take ownership\n");
		#endif
		kfree(dev);
		return -ENODEV;
	}

	mutex_init(&dev->lock);
	dev->usb_dev = usb_dev;
	dev->fan_rpm = 0;
	dev->power = NORMAL;
	dev->product_id = hdev->product;

	laptop = *dev;
	// Internal laptop touchpad found (over USB Bus).
	// Don't bind to it so unload.
	// Only the Keyboard can control the fan speed
	if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_KEYBOARD) {
		kfree(dev);
		return -ENODEV;
	}
	dev_info(&intf->dev, "Found supported device: %s\n", getDeviceDescription(dev->product_id));
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
	// Set all keys to white for now until we get an update from daemon
	for (c=0; c <=5; c++) {
		memset(matrix[c].keys, 0xFF, sizeof(matrix[c].keys));	
	}

	// Now init proc_fs and sysfs
	procfolder = proc_mkdir(PROC_FS_DIR_NAME, NULL);
	if (procfolder == NULL) {
		proc_remove(procfolder);
		hid_err(hdev, "Failed to setup procFS!");
		kfree(dev);
		return -ENOMEM;
	}
	procDaemon = proc_create(PROC_FS_DAEMON_NAME, 0666, procfolder, &proc_fops);

	device_create_file(&hdev->dev, &dev_attr_fan_rpm);
	device_create_file(&hdev->dev, &dev_attr_power_mode);

	// Register LED interface for Keyboard
	led_classdev_register(hdev->dev.parent, &kbd_backlight);
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
	led_classdev_unregister(&kbd_backlight);
	proc_remove(procDaemon);
	proc_remove(procfolder);
	hid_hw_stop(hdev);
	kfree(dev);
	dev_info(&intf->dev, "Razer_laptop_control: Unloaded\n");
}

MODULE_DEVICE_TABLE(hid, table);
static struct hid_driver razer_sc_driver = {
	.name = "Razer laptop System control driver",
	.probe = razer_laptop_probe,
	.remove = razer_laptop_remove,
	.id_table = table,
};

module_hid_driver(razer_sc_driver);