#!/bin/bash

/usr/bin/cp -f 99-razer-laptop-control.rules /etc/udev/rules.d/

echo "IMPORTANT"
echo "ADD YOURSELF TO plugdev GROUP:"
echo ""
echo "sudo usermod -a -G plugdev \$USER"
