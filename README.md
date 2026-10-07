# exg-c-firmware

One Knight image for the exg-c app. NeuroPawn stays a build dependency. This repo does not vendor the library or the linked hex.

| EEPROM 0 | Stream |
| --- | --- |
| 0 or blank | 125 SPS. IMU frames if the sensor answers. A missing IMU stays 57 bytes of zeros. |
| 1 | 250 SPS, EEG only |
| 2 | 500 SPS, EEG only |

After `setup()` the board prints `EXG-FW 1`. The app reads that line and refuses to treat an older image as current.

```bash
./scripts/build.sh
```

Needs [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) and network on the first build. `upload_speed` is 115200. Copy `.pio/build/knight/firmware.hex` to the app firmware folder as `knight.hex`, or let the app build pick it up from that path.

Research hardware. Take the electrodes off before upload. Do not connect the board to mains-powered gear while electrodes are on a person.
