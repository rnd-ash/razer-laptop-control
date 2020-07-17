#!/bin/bash

cargo build --release

sudo /bin/bash << EOF
mkdir -p /usr/share/razercontrol
systemctl stop razerdaemon.service
cp target/release/daemon /usr/share/razercontrol/
cp razerdaemon.service /etc/systemd/system/
systemctl enable razerdaemon.service
systemctl start razerdaemon.service
EOF
echo "Install complete!"
