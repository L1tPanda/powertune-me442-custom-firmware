# Notice: credits and no warranty

## Credits

These patches modify the PowerTune Digital dash application:
<https://github.com/PowerTuneDigital/PowerTuneDigitalOfficial>

PowerTune: Copyright (C) 2016-2018 Markus Ippy, Bastian Gschrey and contributors;
Copyright (C) 2018 PowerTuneDigital. Licensed under the GNU General Public License v3.

The files in `src/` are PowerTune source files with the changes in `patches/` applied. They keep
their original copyright and the GPL-3 licence (see `LICENSE`). The changes are marked
`CUSTOM FIRMWARE` / `EMULATOR` in the code.

"PowerTune" and "PowerTune Digital" are names of PowerTune Digital. "Motorsport Electronics" and
"ME442" are names of Motorsport Electronics. "MX-5" and "Eunos" are Mazda names. They are used only
to say what this works with. **This project is unofficial and is not affiliated with, endorsed by or
supported by PowerTune Digital, Motorsport Electronics or Mazda.**

## No warranty - use at your own risk

This software is provided "as is", without warranty of any kind, express or implied, as set out in
sections 15 and 16 of the GPL-3 licence. In particular:

- **It changes how your engine ECU receives vehicle speed.** Wrong, late or missing speed can affect
  anything the ECU uses speed for (gear detection, gear-based boost or ignition, launch control,
  speed limiters and so on). Check your ECU's behaviour, including what it does when the GPS loses
  its fix, before relying on it.
- **Test on the bench and with the car parked first**, then at low speed. Never adjust or debug it
  while driving.
- GPS speed can be wrong or drop out (tunnels, buildings, poor sky view).
- Installing modified software on your dash may affect your warranty or support from PowerTune
  Digital or Motorsport Electronics. PowerTune's official Update replaces this build.
- Keep a full backup of your dash before installing, and keep the rollback script to hand.

You are responsible for how you use it and for your vehicle complying with the law where you drive.
