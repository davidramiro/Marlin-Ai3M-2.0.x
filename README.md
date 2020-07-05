# Anycubic i3 Mega / Mega-S Marlin 2.0.x by davidramiro

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=3MFT8QMP5ZRCE&source=url)  [![Open Issues](https://img.shields.io/github/issues-raw/davidramiro/Marlin-Ai3M-2.0.x.svg?style=flat)](https://github.com/davidramiro/Marlin-Ai3M-2.0.x/issues?q=is%3Aopen+is%3Aissue) [![Closed Issues](https://img.shields.io/github/issues-closed-raw/davidramiro/Marlin-Ai3M-2.0.x.svg?style=flat)](https://github.com/davidramiro/Marlin-Ai3M-2.0.x/issues?q=is%3Aissue+is%3Aclosed)  [![Travis CI](https://api.travis-ci.org/davidramiro/Marlin-Ai3M-2.0.x.svg?branch=bugfix-2.0.x)](https://travis-ci.org/davidramiro/Marlin-Ai3M-2.0.x)  [![License](https://img.shields.io/github/license/davidramiro/Marlin-Ai3M-2.0.x.svg?style=flat)](https://github.com/davidramiro/Marlin-Ai3M-2.0.x/blob/master/LICENSE)

## Beta build - use with caution!

Additional documentation can be found at the [Marlin Home Page](http://marlinfw.org/).
Please test this firmware and let us know if it misbehaves in any way. Volunteers are standing by!

While I will try to keep this version updated with the latest changes from the Marlin firmware, for now the old readme and Wiki still applies.

---

Looking for a **BLtouch firmware**? Head [this way](https://github.com/MNieddu91/Marlin-AI3M-BLTouch)! Mounting and configuration instructions are included.

#### Make sure to take a look at the [Wiki](https://github.com/davidramiro/Marlin-AI3M/wiki/), especially the [FAQ](https://github.com/davidramiro/Marlin-AI3M/wiki/Frequently-Asked-Questions).

A German translation of the instructions can be found [here](https://kore.cc/i3mega/download/marlin-ai3m_german.pdf).

## Why use this?

- Many people have issues getting the Ultrabase leveled perfectly, using Manual Mesh Bed Leveling the printer generates a mesh of the flatness of the bed and compensates for it on the Z-axis for perfect prints without having to level with the screws.
- Much more efficient bed heating by using PID control. This uses less power and holds the temperature at a steady level. Highly recommended for printing ABS.
- Fairly loud fans, while almost every one of them is easily replaced, the stock FW only gives out 9V instead of 12V on the parts cooling fan so some fans like Noctua don't run like they should. This is fixed in this firmware.
- Even better print quality by adding Linear Advance, S-Curve Acceleration and some tweaks on jerk and acceleration.
- Thermal runaway protection: Reducing fire risk by detecting a faulty or misaligned thermistor.
- Very loud stock stepper motor drivers, easily replaced by Watterott or FYSETC TMC2208. To do that, you'd usually have to flip the connectors on the board, this is not necessary using this firmware.
- No need to slice and upload custom bed leveling tests, test it with a single GCode command
- Easily start an auto PID tune or mesh bed leveling via the special menu (insert SD card, select special menu and press the round arrow)
- Filament change feature enabled: Switch colors/material mid print (instructions below) and control it via display.
- The filament runout, pause and stop functionality have been overhauled and improved: The hotend now parks and retracts (on pause or stop) and purges automatically (on resume).
- Added `M888` cooldown routine for the Anycubic Ultrabase (EXPERIMENTAL): This is meant to be placed at the end Gcode of your slicer. It hovers over the print bed and does circular movements while running the fan. Works best with custom fan ducts.
  - Optional parameters:
  - `T<temperature>`:   Target bed temperature (min 15°C), 30°C if not specified (do not set this under room temperature)
  - `S<fan speed>`:   Fan speed between 0 and 255, full speed if not specified
  - e.g. `M888 S191 T25`: run the fan at 75% until the bed has cooled down to 25°C


## Known issues:

- Power outage support is not included
- Estimated print times from your slicer might be slightly off.
- Special characters on any file or folders name on the SD card will cause the file menu to freeze. Simply replace or remove every special character (Chinese, Arabic, Russian, accents, German & Scandinavian umlauts, ...) from the name. Symbols like dashes or underscores are no problem.
**Important note: On the SD card that comes with the printer there is a folder with Chinese characters in it by default. Please rename or remove it.**

## How to flash this?

I provided three different precompiled hex files: One for no modifications on the stepper motor drivers - good for people who didn't touch anything yet, one for boards with TMC2208 installed and where the connectors have been flipped and one with TMC2208 and the connectors in original orientation.

### Choose your precompiled hex:

- Download the precompiled firmware here: [Releases](https://github.com/davidramiro/Marlin-AI3M/releases)
- Choose the correct hex file:
- For TMC2208 with connectors in original orientation, use `Marlin-AI3M-vx.x.x-TMC2208.hex`
- If you use TMC2208 and already reversed your connectors, use `Marlin-AI3M-vx.x.x-TMC2208_reversed.hex`
- If you use a newer version of the TMC2208 that doesn't require the connector to be reversed (TMC2208 "v2.0" written on the PCB, chip on the top side), please also use `Marlin-AI3M-vx.x.x-TMC2208_reversed.hex`.
- If you use the original stepper motor drivers, use `Marlin-AI3M-vx.x.x-stock_drivers.hex`.

### Or compile it yourself:

- Download and install [Arduino IDE](https://www.arduino.cc/en/main/software)
- Clone or download this repo
- Browse into the Marlin folder and run `Marlin.ino`
- In the IDE, under `Tools -> Board` select `Genuino Mega 2560` and `ATmega2560`
- Open Marlin.ino in the Marlin directory of this repo
- [Customize if needed](https://github.com/davidramiro/Marlin-AI3M/wiki/Customization-&-Compiling) (e.g. motor directions and type at line `559` to `566` and line `857` to `865` in `Configuration.h`)
- Under `Sketch`, select `Export compiled binary`
- Look for the .hex file in the Marlin directory (only use the `Marlin.ino.hex`, not the `Marlin.ino.with_bootloader.hex`!)

### After obtaining the hex file:

- Flash the hex with Cura, OctoPrint or similar
- Use a tool with a terminal (OctoPrint, Pronterface, Repetier Host, ...) to send commands to your printer.
- **Important** Connect to the printer and send the following commands:
- `M502` - load hard coded default values
- `M500` - save them to EEPROM

**If you are using this on a Mega-S, those two additional commands are necessary:**
- `M92 E384` - set correct steps for the new extruder
- `M500` - save them
- I highly recommend calibrating the extruder.

#### Calibration and other instructions have been moved to the [Wiki](https://github.com/davidramiro/Marlin-AI3M/wiki/Calibration).


If you have issues with an uneven bed, this is a great feature.

- Insert an SD card, enter the print menu.
- Enter the special menu by selecting it and pressing the round arrow:

![Special Menu][menu]

- In this menu, the round arrow is used to execute the command you selected.
- Preheat the bed to 60°C with this entry: (if you usually print with a hotter bed, use the Anycubic menu)

![Preheat bed][preheat]

- Level your preheated bed as well as you can with the four screws.
- Start the mesh leveling:

![Start MMBL][start]

- Your nozzle will now move to the first calibration position.
- Don't adjust the bed itself with screws, only use software from here on!
- Use a paper - I recommend using thermopaper like a receipt or baking paper
- Use the onscreen controls to lower or raise your nozzle until you feel a light resistance: (**If you want to send the same command multiple times, select the item again, even though it is still marked red.**)

- Please submit your questions and concerns to the [Issue Queue](https://github.com/MarlinFirmware/Marlin/issues).

## Marlin Support

For best results getting help with configuration and troubleshooting, please use the following resources:

- [Marlin Documentation](http://marlinfw.org) - Official Marlin documentation
- [Marlin Discord](https://discord.gg/n5NJ59y) - Discuss issues with Marlin users and developers
- Facebook Group ["Marlin Firmware"](https://www.facebook.com/groups/1049718498464482/)
- RepRap.org [Marlin Forum](http://forums.reprap.org/list.php?415)
- [Tom's 3D Forums](https://discuss.toms3d.org/)
- Facebook Group ["Marlin Firmware for 3D Printers"](https://www.facebook.com/groups/3Dtechtalk/)
- [Marlin Configuration](https://www.youtube.com/results?search_query=marlin+configuration) on YouTube

## Credits

The current Marlin dev team consists of:

 - Scott Lahteine [[@thinkyhead](https://github.com/thinkyhead)] - USA &nbsp; [Donate](http://www.thinkyhead.com/donate-to-marlin) / Flattr: [![Flattr Scott](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=thinkyhead&url=https://github.com/MarlinFirmware/Marlin&title=Marlin&language=&tags=github&category=software)
 - Roxanne Neufeld [[@Roxy-3D](https://github.com/Roxy-3D)] - USA
 - Chris Pepper [[@p3p](https://github.com/p3p)] - UK
 - Bob Kuhn [[@Bob-the-Kuhn](https://github.com/Bob-the-Kuhn)] - USA
 - João Brazio [[@jbrazio](https://github.com/jbrazio)] - Portugal
 - Erik van der Zalm [[@ErikZalm](https://github.com/ErikZalm)] - Netherlands &nbsp; [![Flattr Erik](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=ErikZalm&url=https://github.com/MarlinFirmware/Marlin&title=Marlin&language=&tags=github&category=software)

## License

Marlin is published under the [GPL license](/LICENSE) because we believe in open development. The GPL comes with both rights and obligations. Whether you use Marlin firmware as the driver for your open or closed-source product, you must keep Marlin open, and you must provide your compatible Marlin source code to end users upon request. The most straightforward way to comply with the Marlin license is to make a fork of Marlin on Github, perform your modifications, and direct users to your modified fork.

While we can't prevent the use of this code in products (3D printers, CNC, etc.) that are closed source or crippled by a patent, we would prefer that you choose another firmware or, better yet, make your own.
