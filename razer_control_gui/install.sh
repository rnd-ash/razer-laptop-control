#!/bin/bash

cargo build --release

sudo /bin/bash << EOF
mkdir -p /usr/share/razercontrol
cp target/release/daemon /usr/share/razercontrol/
cp razerdaemon.service /etc/systemd/system/
systemctl enable razerdaemon.service
systemctl restart razerdaemon.service
EOF
echo "Install complete!"
