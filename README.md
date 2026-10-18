# Custom PowerTune firmware

Patches for the [PowerTune Digital](https://github.com/PowerTuneDigital/PowerTuneDigitalOfficial) dash app
(V5, Raspberry Pi) running with a Motorsport Electronics ME442 ECU on an MX-5:

- the dash's GPS speed is sent to the ME442 as its vehicle-speed input (CAN `0x340`), plus a GPS
  speed/time frame (`0x6A0`);
- a "CUSTOM FW" warning and two-tap confirm on the Update button, because the official updater
  replaces this build.

Unofficial; not affiliated with PowerTune Digital or Motorsport Electronics. Use at your own risk,
and test on the bench first. Licensed GPL-3, like the PowerTune source it modifies (see `LICENSE`).

The patches are made against the source the dash had on 2026-10-17
(`/home/pi/src`, a git checkout of PowerTuneDigital/PowerTuneDigitalOfficial, commit `5380ce2`).

| Patch | What it does |
|---|---|
| `patches/0001-update-button-custom-firmware-warning.patch` | Settings → Network: orange "CUSTOM FW" next to Update; Update needs a second, separate tap within 5 s. **Tested in the car** (also tested in the emulator) |
| `patches/0002-gps-speed-time-on-can.patch` | GPS speed to the ME442 on `0x340` (its vehicle-speed input), plus speed + UTC time on `0x6A0`, at the GPS rate (10 Hz). **Road-tested in the car**: the ME442 reads the GPS speed (also tested in the emulator) |

`src/` holds the edited files for reading; the patches are what gets applied.

## Installing on the dash

**Installed on the dash 2026-10-18** with `install-on-dash.sh` (branch `custom` in `/home/pi/src`, built in
`/home/pi/build-custom`, original app kept as `/opt/PowerTune/PowertuneQMLGui.orig`). Road-tested: the ME442
reads GPS speed from `0x340` (MEITE: VSS Gear Settings, Source = CAN, Mode = km/h).

To (re)install: copy `patches/`, `install-on-dash.sh` and `rollback-on-dash.sh` to
`/home/pi/custom-firmware/` on the dash, then on the dash:

```bash
sh /home/pi/custom-firmware/install-on-dash.sh     # ~6 min on a Pi 4; the dash keeps running
reboot
```

The script refuses to touch a modified `/home/pi/src`, commits the patches on a `custom` branch, builds
in `/home/pi/build-custom` (not `/home/pi/build`, which `powertune-update.sh` would auto-install on boot),
and only replaces the app after a successful build. `sh /home/pi/custom-firmware/rollback-on-dash.sh`
puts the original back. Step-by-step car test: `CAR-TEST.md`.

**The official Update button undoes all of this** (`git reset --hard` + rebuild). Re-run the
install script afterwards.

## GPS frames (patch 0002)

Sent from `GPS::processGPRMC` on every GPS update (10 Hz) on `can0`.

**`0x340` - ME442 vehicle speed** (`ME1_In_1` in ME1_4.dbc). MEITE: Inputs > VSS Gear Settings has
Driven and Undriven Speed Source = CAN.

| Byte | Meaning |
|---|---|
| 0-1 | Vehicle_Speed, km/h / 0.008, little endian (100 km/h = 12500 = `D4 30`) |
| 2-7 | 0 |

Only sent while the GPS has a fix. With no fix nothing is sent rather than 0 km/h, so a dropout can't
look like the car stopped (gear detection, launch control). Check what the ME442 does when the frame stops.

**`0x6A0` - GPS speed and time** (unused id, the ME442 ignores it; for logging or the CAN node)

| Byte | Meaning |
|---|---|
| 0-1 | speed, km/h x 100, little endian (0 when there is no fix) |
| 2 / 3 / 4 / 5 | UTC hours / minutes / seconds / hundredths |
| 6 | 1 = GPS fix valid, 0 = no fix |
| 7 | satellites in view |

In emulator builds the patch also takes NMEA from UDP 45455 and prints frames to UDP 45460 instead of can0
(used with a Windows build of the app for testing; the emulator tools are not in this repo).
