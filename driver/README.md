# Razer laptop control project

A Kernel driver for Razer laptops with RGB keyboards

## Installation

1. Install your linux headers

2. **If you have openrazer installed, you must remove it from your system:**
~~~bash
sudo apt remove openrazer-driver-dkms
~~~

3. DKMS INSTALL INSTRUCTIONS

~~~bash
cd driver
sudo make driver_dkms
sudo dkms add -m razercontrol -v 1.3.0
sudo dkms build -m razercontrol -v 1.3.0
sudo dkms install -m razercontrol -v 1.3.0
~~~
4. After this, you MUST Rebuild your initramfs and reboot:

~~~bash
sudo update-initramfs -u
sudo reboot
~~~

The module will now be persistent.

## Usage
**UPDATE: See the rgb_test_service folder for a easier way to use the system**

### LEGACY WAY VIA SYSFS
1. CD to the following directory:
```
cd "/sys/module/razercontrol/drivers/hid\:Razer\ System\ control\ driver"
```
2. From here, ls shows that there are a couple devices to choose from:
```
ls
0003:1532:0233.0005  0003:1532:0233.0006  0003:1532:0233.0007  bind  module  new_id  uevent  unbind
```
3. cd into one of the first 2 devices (the number doesn't exactly matter, just the first 2 will work where as the third won't). In my case, Ill go to 0003:1532:0233.0005

4. ls now shows the following
```
ls
country  driver  fan_rpm  hidraw  input  modalias  power  power_mode  report_descriptor  subsystem  uevent
```

The 2 files of interest are 'fan_rpm' and 'power_mode'

### fan_rpm
Echo 0 to this to get automatic mode, and any other number to get a desired fan RPM. However, the EC can override this if the manual specified ran rpm is too high or too low.
```
[root@RB-2018 0003:1532:0233.0005]# echo 0 > fan_rpm 
[root@RB-2018 0003:1532:0233.0005]# cat fan_rpm 
Automatic (0)

[root@RB-2018 0003:1532:0233.0005]# echo 5000 > fan_rpm 
[root@RB-2018 0003:1532:0233.0005]# cat fan_rpm 
5000 RPM

[root@RB-2018 0003:1532:0233.0005]# echo 4000 > fan_rpm 
[root@RB-2018 0003:1532:0233.0005]# cat fan_rpm 
4000 RPM

[root@RB-2018 0003:1532:0233.0005]# echo 0 > fan_rpm 
[root@RB-2018 0003:1532:0233.0005]# cat fan_rpm 
Automatic (0)

```

### power_mode
Echo 0 to this file for balanced mode and 1 for gaming mode (increase GPU/CPU TDP)
```
[root@RB-2018 0003:1532:0233.0005]# echo 0 > power_mode
[root@RB-2018 0003:1532:0233.0005]# cat power_mode 
Balanced (0)

[root@RB-2018 0003:1532:0233.0005]# echo 1 > power_mode
[root@RB-2018 0003:1532:0233.0005]# cat power_mode
Gaming (1)
```

On some laptops, there is also another mode called creator mode. This ramps up CPU power instead of GPU power, this can be enabled as shown:
```
[root@RB-2018 0003:1532:0233.0005]# echo 2 > power_mode
[root@RB-2018 0003:1532:0233.0005]# cat power_mode
Creator (2)
```

NOTE: Turning on gaming mode can automatically make the fan increase in speed as the EC seems to switch to a more aggressive fan curve if still in automatic mode.

### DKMS REMOVE INSTRUCTIONS
```
sudo dkms remove razercontrol -v 1.3.0 --all
```
