#!/bin/bash

set -e

PATH='/sbin:/bin:/usr/sbin:/usr/bin'

if [ -x /usr/bin/logger ]; then
    LOGGER=/usr/bin/logger
elif [ -x /bin/logger ]; then
    LOGGER=/bin/logger
else
    unset LOGGER
fi

if [ -t 1 -a -z "$LOGGER" ] || [ ! -e '/dev/log' ]; then
    mesg() {
        echo "$@" >&2
    }
elif [ -t 1 ]; then
    mesg() {
        echo "$@"
        $LOGGER -t "${0##*/}[$$]" "$@"
    }
else
    mesg() {
        $LOGGER -t "${0##*/}[$$]" "$@"
    }
fi

mesg "Driver $1"
mesg "Device_ID $2"
/usr/bin/chmod 666 "/sys$1/fan_rpm"
/usr/bin/chmod 666 "/sys$1/power_mode"
/usr/bin/chmod 666 "/sys$1/brightness"
/usr/bin/chmod 666 "/sys$1/key_colour_map"