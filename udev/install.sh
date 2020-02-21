#!/bin/bash

/usr/bin/cp -f 99-razerlc.rules razer_perms.sh /etc/udev/rules.d/
chmod +x /etc/udev/rules.d/razer_perms.sh
udevadm control --reload-rules && udevadm trigger