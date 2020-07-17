# Razer laptop control project
Designed to be a replacment driver to openrazer project with additional support for Razer laptops fan and power mode modifications.

# Join the unofficial Linux Razer Channel:
[Discord link](https://discord.gg/GdHKf45)

# DEMO VIDEOS:
[Playlist of all demos can be viewed here](https://www.youtube.com/playlist?list=PLxrw-4Vt7xtsO21RxaDwd7GJlKs3YU-g4)

## Repo contents
### Driver
Kernel module required for the software to work

## razer_control_gui
** Experimental**
Test suite for Rust based daemon, CLI and GUI for controlling the driver, and setting custom keyboard effects

# Changelog

## 1.2.1 - 17/07/2020
* Added initial rust based Daemon - see razer_control_gui for more details
* Removed useless printk calls in kernel module - DMESG output should not longer be cluttered

## 1.1.0 - 08/07/2020
* Re-wrote the kernel driver (Made my life easier for the future)
* Added native ledfs backlight control (You should now be able to control the keyboard backlight via gnome/KDE Desktop)

## 1.0.0
* Initial release
