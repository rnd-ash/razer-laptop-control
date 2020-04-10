#!/bin/bash
rmmod "razerkbd"
rmmod "razercontrol"
sleep 5
insmod "src/razercontrol.ko"

