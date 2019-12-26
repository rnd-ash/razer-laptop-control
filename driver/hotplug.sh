#!/bin/bash

port="1-8" # as shown by lsusb -t: {bus}-{port}(.{subport})

echo "$port" > /sys/bus/usb/drivers/usb/unbind
sleep 1
echo "$port" > /sys/bus/usb/drivers/usb/bind
