# Custom PowerTune firmware

Patches for the [PowerTune Digital](https://github.com/PowerTuneDigital/PowerTuneDigitalOfficial) dash app
(V5, Raspberry Pi) running with a Motorsport Electronics ME442 ECU on an MX-5:

- the dash's GPS speed is sent to the ME442 as its vehicle-speed input (CAN `0x340`), plus a GPS
  speed/time frame (`0x6A0`);
- a "CUSTOM FW" warning and two-tap confirm on the Update button, because the official updater
  replaces this build;
- on-screen buttons that send their state to the ECU on CAN (`0x6A1`), e.g. a map switch that
  changes colour with the map, and the gear reading showing `N` for neutral.

Unofficial; not affiliated with PowerTune Digital or Motorsport Electronics. Use at your own risk,
and test on the bench first. Licensed GPL-3, like the PowerTune source it modifies (see `LICENSE`).

> **No warranty - use at your own risk.** This changes how your ECU gets vehicle speed. Test on the
> bench and parked first; it may affect your dash warranty or support. Credits to the PowerTune authors
> and the full warning: [NOTICE.md](NOTICE.md).

The patches are made against the source the dash had on 2026-10-17
(`/home/pi/src`, a git checkout of PowerTuneDigital/PowerTuneDigitalOfficial, commit `5380ce2`).

| Patch | What it does |
|---|---|
| `patches/0001-update-button-custom-firmware-warning.patch` | Settings → Network: orange "CUSTOM FW" next to Update; Update needs a second, separate tap within 5 s. **Tested in the car** (also tested in the emulator) |
| `patches/0002-gps-speed-time-on-can.patch` | GPS speed to the ME442 on `0x340` (its vehicle-speed input), plus speed + UTC time on `0x6A0`, at the GPS rate (10 Hz). **Road-tested in the car**: the ME442 reads the GPS speed (also tested in the emulator) |
| `patches/0003-can-buttons-and-gear-n.patch` | "CAN button 1-4" data sources: a State gauge using one becomes a tap-to-toggle button, sent to the ECU on `0x6A1` at 10 Hz. Text labels showing Gear show `N` for 0. **Tested on the dash in the car** (2026-11-08): taps toggle the buttons and `0x6A1` is on the bus (also tested in the emulator) |
| `patches/0004-can-buttons-sync-with-relay-box.patch` | Needs 0003. A CAN relay box keeps the button states (through power cycles) and reports them on `0x6A2`; the dash copies them onto its buttons, so the outputs are right before the dash has booted. Adds "CAN input 1-4" sources: signals the box senses, e.g. whether the ECU is running the fan. **Tested in the emulator** with a fake relay box (2026-11-15): states come back after a dash restart, taps are followed and saved. Not yet on the dash |

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

## CAN buttons (patch 0003)

On a dash page, add a **State gauge**, set its **Source** to **CAN button 1** (to 4) and pick an OFF and an
ON picture. Outside edit mode a tap switches it between off and on; the picture follows, and the dash
sends the state to the ECU. The trigger value is ignored for CAN buttons (ON = 1). Every button is OFF
when the dash starts. `tools/make_button_images.py` in
[powertune-page-editor](https://github.com/L1tPanda/powertune-page-editor) makes pill-shaped OFF/ON pictures.

**`0x6A1` - dash buttons** (11-bit id, 8 bytes, 10 Hz and immediately on every tap)

| Byte | Meaning |
|---|---|
| 0 | CAN button 1: 0 = off, 1 = on (page 1: map switch, off = MAP 1, on = MAP 2) |
| 1 | CAN button 2 (page 1: fan override, off = FAN AUTO, on = FAN ON) |
| 2 / 3 | CAN buttons 3 / 4 |
| 4 | bit 0 = 1 once the dash has heard the relay box and shows its states (patch 0004) |
| 5-6 | 0 |
| 7 | counter, +1 every frame (wraps at 255) |

**`0x6A2` - relay box to dash** (patch 0004; 11-bit id, 8 bytes, 10 Hz)

| Byte | Meaning |
|---|---|
| 0-3 | the button states the box has saved and is driving (0 / 1) |
| 5 | sensed inputs: bit 0 = "CAN input 1" ... bit 3 = "CAN input 4" |
| 7 | counter |

With patch 0004 the relay box owns the states. It restores them from its flash and switches its
outputs as soon as it powers up. The dash sends byte 4 = 0 until it has heard `0x6A2`, and the box
ignores those frames, so a dash that is still booting (all buttons off) can't undo the saved states.
Once synced, a tap changes the state and the box follows and saves it. Without a box, the buttons start
off and work as in 0003. For a "CAN input" indicator, add a State gauge with source **CAN input N**
(ON when the input is active); inputs read off if the box stops sending for 1 s.

**The ME442 can't read `0x6A1` itself on firmware 4.2.1 (definition 62).** Custom CAN inputs need a
Gen2B/Gen2D ECU on definition 83 or newer, which isn't published yet; on 4.2.1 the only CAN inputs
are the fixed presets (e.g. ME1_3) and vehicle speed. Until then, a small CAN relay node (e.g. an ESP32
board with two relays and a CAN transceiver) can read `0x6A1` and switch the ECU's map-select input
(Dig In 5, active low) and the fan relay. With patch 0004 the box keeps the states itself (see
`0x6A2` below); with 0003 alone, make it release both relays if the frame stops for about 0.5 s.
