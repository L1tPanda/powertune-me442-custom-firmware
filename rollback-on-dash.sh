#!/bin/sh
# Put the original PowerTune app back (saved by install-on-dash.sh), then reboot.
# Run on the dash:  sh /home/pi/custom-firmware/rollback-on-dash.sh
set -e
APP=/opt/PowerTune/PowertuneQMLGui
[ -f "$APP.orig" ] || { echo "No $APP.orig - nothing to roll back"; exit 1; }
cp "$APP.orig" "$APP.new"
mv "$APP.new" "$APP"
echo "Original app restored. Rebooting..."
sync
reboot
