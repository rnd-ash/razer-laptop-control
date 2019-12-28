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

#define SYS_CONTROL_ID 4 // System control
#define CONS_CONSTROL_ID 3 // Consumer control

struct razer_laptop {
    int product_id;
    struct usb_device *usb_dev;
    struct mutex lock;
    int fan_rpm;
    int gaming_mode;
};

static ssize_t get_fan_rpm(struct device *dev, struct device_attribute *attr, char *buf) {
   struct razer_laptop *laptop = dev_get_drvdata(dev);
    if (laptop->fan_rpm == 0) {
        return sprintf(buf, "%s", "Automatic (0)\n");
    } else {
        return sprintf(buf, "%d RPM\n", laptop->fan_rpm);
    }

}

static ssize_t get_performance_mode(struct device *dev, struct device_attribute *attr, char *buf) {
    struct razer_laptop *laptop = dev_get_drvdata(dev);
    if (laptop->gaming_mode == 0) {
        return sprintf(buf, "%s", "Balanced (0)\n");
    } else {
        return sprintf(buf, "%s", "Gaming(1)\n");
    }
}

void crc(char * buffer) {
    int res = 0;
    int i;
    for (i = 2; i < 88; i++) {
        res ^= buffer[i];
    }
    buffer[88] = res;
}


int send_payload(struct usb_device *usb_dev, void const *buffer, unsigned long minWait, unsigned long maxWait) {
    crc(buffer);
    char * buf2;
    buf2 = kmemdup(buffer, sizeof(char[90]), GFP_KERNEL);
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
    usleep_range(minWait,maxWait);
    kfree(buf2);
    return 0; // 0 = OK, 1 = Not correct;
}

static ssize_t set_fan_rpm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct razer_laptop *laptop = dev_get_drvdata(dev);
    unsigned long x;
    __u8 request_fan_speed;
    char buffer[90];
    memset(buffer, 0x00, sizeof(buffer));
    if (kstrtol(buf, 10, &x))
        return -EINVAL;

    if (x != 0) {
        request_fan_speed = clampFanRPM(x, laptop->product_id);
        hid_err(laptop->usb_dev, "Requesting MANUAL fan at %d RPM", ((int) request_fan_speed * 100));
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
        hid_err(laptop->usb_dev, "Requesting AUTO Fan");
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

static ssize_t set_performance_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct razer_laptop *laptop = dev_get_drvdata(dev);
    unsigned long x;
    if (kstrtol(buf, 10, &x))
        return -EINVAL;
    if (x == 1 || x == 0){
        laptop->gaming_mode = x;
        if (x == 1)
            hid_err(laptop->usb_dev,"%s", "Requesting Gaming performance");
        else if (x == 0)
            hid_err(laptop->usb_dev,"%s", "Requesting Balanced performance");
        char buffer[90];
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
        buffer[9] = 0x02;
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
    dev->fan_rpm = 0;
    dev->gaming_mode = 0;
    dev->product_id = hdev->product;
    if (intf->cur_altsetting->desc.bInterfaceProtocol != USB_INTERFACE_PROTOCOL_KEYBOARD) {
        hid_err(hdev, "Found mouse - Unloading for device!\n");
        kfree(dev);
        return 0;
    }
    device_create_file(&hdev->dev, &dev_attr_fan_rpm);
    device_create_file(&hdev->dev, &dev_attr_power_mode);
    
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
    device_remove_file(&hdev->dev, &dev_attr_power_mode);
    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer_laptop_control: Unloaded on %s\n",&intf->dev.init_name);
}

static const struct hid_device_id table[] = {
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2018_ADV)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2018_BASE)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_ADV)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_BASE)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_MID)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2017_STEALTH_END)},
    { HID_USB_DEVICE(RAZER_VENDOR_ID, BLADE_2019_STEALTH)},
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