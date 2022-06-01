# Razer laptop control project
A standalone driver + application designed for Razer notebooks

## NOTICE ##
This repo is now archived as I have moved over to System76 after my trusty blade 15 died!

The work continues however, do check [phush0's](https://github.com/phush0/razer-laptop-control-no-dkms) repository where he is adding more features and continuing support!

## Join the unofficial Razer linux Channel
I can be contacted on this discord server under the 'laptop-control' channel
[Discord link](https://discord.gg/GdHKf45)

## Project demo videos:
[Playlist of all demos can be viewed here](https://www.youtube.com/playlist?list=PLxrw-4Vt7xtsO21RxaDwd7GJlKs3YU-g4)

## Install
### Arch linux
Use the 2 PKGBUILDS located here:
* [DKMS Driver](https://aur.archlinux.org/packages/razer-laptop-control-dkms-git/)
* [CLI and Daemon](https://aur.archlinux.org/packages/razer-laptop-control-git/)

### Fedora Workstation (tested on 34, 35, 36)
#### Requirements
* dkms: `sudo dnf install dkms`
* appropriate kernel headers (& possibly kernel-devel): `sudo dnf install kernel-headers kernel-devel`
* cargo: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`

#### Driver Installation
1. Make and install dkms driver.
```
cd driver
sudo make driver_dkms
sudo dkms add -m razercontrol -v 1.3.0
sudo dkms build -m razercontrol -v 1.3.0
sudo dkms install -m razercontrol -v 1.3.0
```
2. Rebuild initramfs.
```
sudo dracut -v -f
```
3. Reboot your computer.

#### Front-end Installation
1. Compile and install `razer_control_gui`.
```
cd razer_control_gui
./install.sh
```
2. Check whether razer laptop control daemon is enabled and running.
```
systemctl status razerdaemon.service
```
3. To enable it (run automatically)
```
systemctl enable razerdaemon.service
```
4. If the daemon is running without `exit-code`, enjoy using it!
5. If the daemon fail to run, you might have secure-boot turned on. Refer [here](#ATTENTION).
#### 

### Other distros
Unfortunatly, you have to build from scrath.

## What does this control
On all razer notebooks, the following is supported:
* RGB keyboard control (Experimental)
* Fan control
* Power control

### RGB control
Currently, there are various build in effects to choose from, and multiple 'effect layers' can be stacked on top of eachother, allowing for each key to be controlled with its own effect!
### Fan control
Control the notebooks fan RPM just like in Synapse! - Currently supports switching between Automatic and manual (Up to 5300 RPM)
### Power control
Control the power target of the notebook just like in Synapse!
* Balanced - Standard 35W CPU TDP
* Gaming - 55W CPU TDP - Allows for higher and more sustained CPU boost clocks (Additionally fan RPMs are increased)
* Creator (Select notebooks only!) - Allows for higher GPU TDP

## Repo contents
### Driver
Kernel module required for the software to work

### razer_control_gui
Experimental code for system daemon and UI/CLI interface for controlling both RGB aspects and fan+Power subsystem of razer notebooks

## ATTENTION
If the daemon fail to run, and the log message says "Timed out waiting for sysfs after a minute!", you most likely have secure-boot turned on.
To get it running on secure-boot enabled devices, you must sign the kernel module yourself. Proceed with caution. **We are NOT reliable for any
damages done to your computer**.

### Requirements
* openssl: `sudo dnf install openssl`
* mokutil: `sudo dnf install mokutil`

### Signing kernel module
1. Generate a signing key. Replace `~/` to any path that you will remember.
```
sudo openssl req -new -x509 -newkey rsa:2048 -keyout ~/private.key -outform DER -out ~/public.der -nodes -days 36500 -subj "/CN=Razer Laptop Control"
```
2. Import the key. Replace `~/` with the location of where you saved your key. Note that you can set any password you want (just remember it).
```
sudo mokutil --import ~/public.der
```
3. Determine where the driver is located at (most likely in `/lib/modules/$(uname -r)/extra/`).
```
cd /lib/modules/$(uname -r)/extra/
ls
```
4. You should see `razercontrol.ko` or `razercontrol.ko.xz`. If the driver ends with `.ko`, proceed to step `6`.
5. Decompress the kernel module.
```
sudo unxz razercontrol.ko.xz
```
6. Sign the module. Don't forget to replace `~/` to the correct path if you chose to save the keys somewhere else.
```
sudo /usr/src/kernels/$(uname -r)/scripts/sign-file sha256 ~/private.key ~/public.der razercontrol.ko
```
7. Skip to step `8` if you skipped step `6`. If you had to decompress the module, you must recompress it again.
```
sudo xz -f razercontrol.ko
```
8. Reboot.
9. At reboot, `mokutil` will appear (blue screen). Choose `Enroll MOK` -> `Continue` -> type in the password that you have set in step `2`.
10. VOILA! You have signed your module. The daemon should be running without failing. Try:
```
razer-cli read fan
```

### Side Note
The key will expire after 36500 days. If you've managed to live that long, you will have to generate a new key, and sign the module using the new key.

## Changelog

### 1.3.0 - 04/01/2021
* Add support for Razer book 2020
* Add support for Razer blade pro 2020 FHD

### 1.2.1 - 17/07/2020
* Added initial rust based Daemon - see razer_control_gui for more details
* Removed useless printk calls in kernel module - DMESG output should not longer be cluttered

### 1.1.0 - 08/07/2020
* Re-wrote the kernel driver (Made my life easier for the future)
* Added native ledfs backlight control (You should now be able to control the keyboard backlight via gnome/KDE Desktop)

### 1.0.0
* Initial release
