#include <Arduino.h>
#include <EEPROM.h>
#include <NeuroPawn.h>
#include <avr/wdt.h>
#include <string.h>

/* EEPROM byte 0 is the boot mode. Settings sends exgmode_N over USB.
 * The sketch stores N and resets so setup() runs once with that rate.
 * The bootloader is not involved.
 *
 *   0 or blank  125 SPS, IMU if the chip answers
 *   1           250 SPS, EEG only
 *   2           500 SPS, EEG only
 *
 * Channel and bias lines (chon_, choff_, rldadd_, rldremove_) stay with
 * the library. Only a line that starts with 'e' is ours.
 */
#ifndef EXG_FW_VERSION
#define EXG_FW_VERSION 2
#endif

static char cmd[12];
static uint8_t cmd_n;

static uint8_t boot_mode(void)
{
    uint8_t m = EEPROM.read(0);
    if (m > 2) {
        return 0;
    }
    return m;
}

static void apply_mode(uint8_t m)
{
    if (m == 1) {
        neuropawn.setup(NP_DEFAULT, NP_SPS_250);
    } else if (m == 2) {
        neuropawn.setup(NP_DEFAULT, NP_SPS_500);
    } else {
        neuropawn.setup(NP_IMU, NP_SPS_125);
    }
    Serial.print(F("EXG-FW "));
    Serial.println(EXG_FW_VERSION);
    Serial.print(F("EXG-MODE "));
    Serial.println((int)m);
}

static void reboot_into(uint8_t m)
{
    EEPROM.update(0, m);
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
}

static void poll_mode_line(void)
{
    while (Serial.available() > 0) {
        int c = Serial.peek();
        if (cmd_n == 0 && c != 'e') {
            return;
        }
        c = Serial.read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            cmd[cmd_n] = 0;
            if (cmd_n == 9 && memcmp(cmd, "exgmode_", 8) == 0 && cmd[8] >= '0' && cmd[8] <= '2') {
                reboot_into((uint8_t)(cmd[8] - '0'));
            }
            cmd_n = 0;
            return;
        }
        if (cmd_n < (uint8_t)(sizeof(cmd) - 1)) {
            cmd[cmd_n++] = (char)c;
        } else {
            cmd_n = 0;
        }
    }
}

void setup(void)
{
    apply_mode(boot_mode());
}

void loop(void)
{
    poll_mode_line();
    neuropawn.acquire_data();
}
