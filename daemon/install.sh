#!/bin/bash

/usr/bin/cp razercontrol-daemon /usr/bin/
chmod +x /usr/bin/razercontrol-daemon
/usr/bin/cp razerlaptop-control.service /etc/systemd/system/
systemctl enable razerlaptop-control
systemctl start razerlaptop-control