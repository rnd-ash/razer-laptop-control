# Razer laptop control project

*I didn't fancy modifying the openrazer driver, so have something different*

## What is this?
This is a test driver I wrote in about 20 hours to allow users with Razer laptops to control their fan speed and power mode under Linux (Because using a VM with Synapse is stupid).

## How to use
**You Need Linux headers installed first!**

**If you have openrazer installed, you MUST unload it first.**
```
sudo rmmod razerkbd
```

### Build / Install instructions
```
cd driver
make
cd src
sudo insmod razercontrol.ko
```

## OK Driver installed, how do I use this?
1. CD to the following directry:
```
cd "/sys/module/razercontrol/drivers/hid:Razer laptop System control driver"
```
2. From here, ls shows that there are a couple devies to choose from:
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
[Balanced (0)] Gaming (1)

[root@RB-2018 0003:1532:0233.0005]# echo 1 > power_mode
[root@RB-2018 0003:1532:0233.0005]# cat power_mode
Balanced (0) [Gaming (1)]
```

NOTE: Turning on gaming mode can automatically make the fan increase in speed as the EC seems to switch to a more aggressive fan curve if still in automatic mode.
NOTE2: Switching modes may reset your current fan_rpm setting to auto mode.
