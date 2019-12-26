#!/bin/bash
# Save this script as /usr/local/sbin/a4
set -euo pipefail

if [[ -z ${1-} ]]; then
	echo "You need to run this tool with a .pcapng file as a parameter."
	echo "The file has to be in the same folder as your current pwd - $(pwd)"
	echo "Example: a4 captureCalibrationIntoPreset.pcapng"
	echo "Result will be copied to your clipboard, you should paste it into https://a4.rys.pw"
	exit 1
fi
filename=$1
echo "$filename" > "/tmp/$filename.txt"
tcpdump -r "$filename" -x >> "/tmp/$filename.txt" 2>/dev/null # Useless line is printed to STDERR
sed -i s/"	0x0000:  "//g "/tmp/$filename.txt"
sed -z -i s/"\n	0x00[a-f0-9][a-f0-9]:  "/" "/g "/tmp/$filename.txt"
cat "/tmp/$filename.txt" | xclip -i -selection c
rm "/tmp/$filename.txt"
echo "Success! Result copied to clipboard, you should paste it into https://a4.rys.pw"

