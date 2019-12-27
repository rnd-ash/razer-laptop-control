#include <linux/hid.h>
#include <linux/usb/input.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fancontrol.h"


MODULE_AUTHOR("Ashcon Mohseninia");
MODULE_DESCRIPTION("Razer system control driver for laptops");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

#define SYS_CONTROL_ID 4 // System control
#define CONS_CONSTROL_ID 3 // Consumer control

struct razer_laptop {
    struct usb_device *usb_dev;
    struct mutex lock;
    __u8 fan_rpm;
    bool is_manual_fan;
    bool is_gaming_mode;
};

static ssize_t get_fan_rpm(struct device *dev, struct device_attribute *attr, char *buf) {
    return "UNknonw";
}

int send_payload(struct device *dev, void const *buffer) {
    struct usb_device *usb_dev = interface_to_usbdev(to_usb_interface(dev->parent));
    hid_err(usb_dev, "%d", sizeof(buffer));
    char * buf2;
    buf2 = kmemdup(buffer, sizeof(char[90]), GFP_KERNEL);
    hid_err(usb_dev, "Sending payload to Controller\n");
    int len;
    len = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
        0x09,
        0x21,
        0x0300,
        0x0002,
        buf2,
        90,
        USB_CTRL_SET_TIMEOUT
    );
    hid_err(usb_dev, "Sent to Controller!\n");
    usleep_range(600,800);
    kfree(buf2);
    return 0; // 0 = OK, 1 = Not correct;
}

void crc(char * buffer) {
    int res = 0;
    int i;
    for (i = 2; i < 88; i++) {
        res ^= buffer[i];
    }
    buffer[88] = res;
}

static ssize_t set_fan_rpm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct usb_device *usb_dev = interface_to_usbdev(to_usb_interface(dev->parent));
    unsigned long x;
    if (kstrtol(buf, 10, &x))
        return -EINVAL;
    if (x == 0) { // Request auto fan! 
        hid_err(usb_dev, "%s" ,"Requesting AUTO fan");
        char buffer[90] = {0x00, 0x1f, 0x00, 0x00, 0x00, 0x04, 0x0d, 0x02, 0x00, 0x02, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        crc(buffer);
        send_payload(dev, buffer);
        return count;
    } else {
        __u8 request_fan_speed = clampFanRPM(x);
        hid_err(usb_dev, "Requesting MANUAL fan at %d RPM", ((int) request_fan_speed * 100));
        char buffer[90] = {0x00, 0x1f, 0x00, 0x00, 0x00, 0x04, 0x0d, 0x02, 0x00, 0x02, 0x00,
                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        crc(buffer);
        send_payload(dev, buffer);
        buffer[5] = 0x03;
        buffer[7] = 0x01;
        buffer[8] = 0x00;
        buffer[9] = 0x01;
        buffer[10] = request_fan_speed;
        buffer[11] = 0x00;
        crc(buffer);
        send_payload(dev, buffer);
        return count;
    }
    return count;
}


static DEVICE_ATTR(fan_rpm, 0664, get_fan_rpm, set_fan_rpm);

// Called on load module
static int razer_laptop_probe(struct hid_device *hdev, const struct hid_device_id *id) {
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    dev_info(&intf->dev, "razer_laptop_control: Supported device found!\n");
    struct razer_laptop *dev = NULL;
    dev = kzalloc(sizeof(struct razer_laptop), GFP_KERNEL);



    if (dev == NULL) {
        dev_err(&intf->dev, "Out of memory!\n");
        return -ENOMEM;
    }
    printk(KERN_DEBUG, "%s", &dev->usb_dev->descriptor);
    mutex_init(&dev->lock);
    dev->usb_dev = usb_dev;
    
    if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_KEYBOARD) {
        hid_err(hdev, "Found mouse - Unloading for device!\n");
        kfree(dev);
        return 0;
    }
    device_create_file(&hdev->dev, &dev_attr_fan_rpm);
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
    device_remove_file(&hdev->dev, &dev_attr_fan_rpm);
    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer_laptop_control: Unloaded on %s\n",&intf->dev.init_name);
}

static const struct hid_device_id table[] = {
    { HID_USB_DEVICE(0x1532, 0x0233)}, // Blade 2018 ADV
    { HID_USB_DEVICE(0x1532, 0x023a)}, // Blade 2019
    { HID_USB_DEVICE(0x1532, 0x0232)}, // Late 2017 stealth
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