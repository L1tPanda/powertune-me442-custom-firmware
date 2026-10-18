# In-car test: custom firmware (update warning + GPS speed to the ME442)

## Before you start
- Car outside with open sky (GPS needs a fix), dash on WiFi.
- **Engine running, or a battery charger on**: the build runs on the dash for a while (likely 10-30 min).
- Laptop with MEITE connected to the ECU (to see the speed it receives).
- MEITE: Inputs > VSS Gear Settings already has Driven/Undriven Speed Source = CAN.

## Install (over SSH)
1. Save the dash's settings, check the source is unmodified.
2. Copy `patches/` + scripts to `/home/pi/custom-firmware/`.
3. Run `install-on-dash.sh` in the background: branch `custom`, build in `/home/pi/build-custom`,
   install only if the build succeeds (original app kept as `/opt/PowerTune/PowertuneQMLGui.orig`).
4. Reboot the dash.

## Checks
1. Dash comes up as normal, page 1 and page 2 unchanged.
2. Settings > Network shows orange **CUSTOM FW**; Update needs two separate taps.
3. With a GPS fix: `candump can0,340:7FF,6A0:7FF` shows both frames ~10/s.
   Parked = 0 km/h in `0x340` (that's correct, not a fault).
4. MEITE live: VSS Gear / vehicle speed follows the GPS speed. Drive slowly first.
5. Note what the ME442 does if the GPS loses its fix (`0x340` stops being sent).

## If anything is wrong
`sh /home/pi/custom-firmware/rollback-on-dash.sh` puts the original app back and reboots.
Take a full backup of the dash (e.g. a tar of / over SSH) before installing.
