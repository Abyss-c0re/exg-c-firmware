# exg-c-firmware

One Knight image for the exg-c app. NeuroPawn stays a build dependency. This repo does not vendor the library or the linked hex.

| EEPROM 0 | Stream |
| --- | --- |
| 0 or blank | 125 SPS. IMU frames if the sensor answers. A missing IMU stays 57 bytes of zeros. |
| 1 | 250 SPS, EEG only |
| 2 | 500 SPS, EEG only |

After `setup()` the board prints `EXG-FW 4` and `EXG-MODE N`. `exgmode_N` is read before the NeuroPawn command parser, which drops any other line. A partial line is dropped on a later call, 20 ms on. `available()` does not wait. Firmware 3 already answered `exgmode_2` with `EXG-SWITCH 2` and then `EEG 500 SPS`. Firmware 4 keeps that, and stops waiting inside `available()` while the line is incomplete.

Settings sends one line on the same 115200 link, then the board restarts itself:

```
exgmode_0
exgmode_1
exgmode_2
```

`chon_`, `choff_`, `rldadd_`, and `rldremove_` are unchanged. One upload of this image is required before those three lines do anything. Later mode changes do not open the bootloader.

```bash
./scripts/build.sh
```

Needs [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) and network on the first build. `upload_speed` is 115200. Copy `.pio/build/knight/firmware.hex` to the app firmware folder as `knight.hex`, or let the app build pick it up from that path.

Research hardware. Take the electrodes off before upload. Do not connect the board to mains-powered gear while electrodes are on a person.
