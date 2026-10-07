#include <Arduino.h>
#include <EEPROM.h>
#include <NeuroPawn.h>

/* EEPROM byte 0, written by the exg-c flasher before reset:
 *   0 or blank  125 SPS, IMU if the chip answers (57 bytes, zeros if absent)
 *   1           250 SPS, EEG only
 *   2           500 SPS, EEG only
 * setup() runs once. The boot line EXG-FW N is the version the app requires.
 */
#ifndef EXG_FW_VERSION
#define EXG_FW_VERSION 1
#endif

static uint8_t boot_mode(void)
{
    uint8_t m = EEPROM.read(0);
    if (m > 2) {
        return 0;
    }
    return m;
}

void setup(void)
{
    uint8_t m = boot_mode();
    if (m == 1) {
        neuropawn.setup(NP_DEFAULT, NP_SPS_250);
    } else if (m == 2) {
        neuropawn.setup(NP_DEFAULT, NP_SPS_500);
    } else {
        neuropawn.setup(NP_IMU, NP_SPS_125);
    }
    Serial.print("EXG-FW ");
    Serial.println(EXG_FW_VERSION);
}

void loop(void)
{
    neuropawn.acquire_data();
}
