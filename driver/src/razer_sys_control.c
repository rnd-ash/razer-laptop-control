#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

MODULE_AUTHOR("Ashcon Mohseninia");
MODULE_DESCRIPTION("Razer system control driver for laptops");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

struct razer_device {
    struct usb_device *usb_dev;
    struct mutex lock;
};

static ssize_t get_performance_mode(struct device *dev, struct device_attribute *attr, char *buf) {
    return sprintf(buf, "%d", 1);
}
static ssize_t set_performance_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    return 0;
}


static DEVICE_ATTR(performance_mode, 0664, get_performance_mode, set_performance_mode);

// Called on load module
static int razer_laptop_probe(struct hid_device *hdev, const struct hid_device_id *id) {
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    dev_info(&intf->dev, "razer_laptop_control: Supported device found!\n");
    struct razer_device *dev = NULL;
    dev = kzalloc(sizeof(struct razer_device), GFP_KERNEL);



    if (dev == NULL) {
        dev_err(&intf->dev, "Out of memory!\n");
        return -ENOMEM;
    }
    printk(KERN_DEBUG, "%s", &dev->usb_dev->descriptor);
    mutex_init(&dev->lock);
    dev->usb_dev = usb_dev;
    device_create_file(&hdev->dev, &dev_attr_performance_mode);
    usb_disable_autosuspend(usb_dev);
    hid_err(hdev, "%s", usb_dev->name);

    // TODO Bind only to System control module. Not keyboard and Consumer control module.

    // Currently binding to all these addresses
    // input: Razer Razer Blade Keyboard as /devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.1/0003:1532:0233.0006/input/input188
    // input: Razer Razer Blade Consumer Control as /devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.1/0003:1532:0233.0006/input/input189
    // input: Razer Razer Blade System Control as /devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.1/0003:1532:0233.0006/input/input190
    hid_set_drvdata(hdev, dev);
    if(hid_parse(hdev)) {
        hid_err(hdev, "Failed to parse device!\n");
        kfree(dev);
        return 1;
    }
    if(hid_hw_start(hdev, HID_CONNECT_DEFAULT)) {
        hid_err(hdev, "Failed to start device!\n");
        kfree(dev);
        return 1;
    }

    printk(KERN_INFO, "Razer_laptop_control: Loaded\n");
    return 0;
}

// Called on unload module
static void razer_laptop_remove(struct hid_device *hdev) {
    struct device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    dev = hid_get_drvdata(hdev);
    device_remove_file(&hdev->dev, &dev_attr_performance_mode);
    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer_laptop_control: Unloaded on %s\n",&intf->dev.init_name);
}

static const struct hid_device_id table[] = {
    { HID_USB_DEVICE(0x1532, 0x0233)},
    { }
};
MODULE_DEVICE_TABLE(hid, table);
static struct hid_driver razer_sc_driver = {
    .name = "Razer System control driver",
    .probe = razer_laptop_probe,
    .remove = razer_laptop_remove,
    .id_table = table,
};
module_hid_driver(razer_sc_driver);