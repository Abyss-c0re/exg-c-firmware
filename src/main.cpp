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
 * the library. acquire_data() calls Stream::readString() on any pending
 * byte and drops a line it does not know. A line that starts with 'e'
 * is taken from HardwareSerial::available() before that read.
 */
#ifndef EXG_FW_VERSION
#define EXG_FW_VERSION 3
#endif

static char cmd[12];
static uint8_t cmd_n;

extern "C" int __real__ZN14HardwareSerial9availableEv(HardwareSerial *self);

static int serial_waiting(void)
{
    return __real__ZN14HardwareSerial9availableEv(&Serial);
}

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
    Serial.print(F("EXG-SWITCH "));
    Serial.println((int)m);
    Serial.flush();
    wdt_enable(WDTO_15MS);
    for (;;) {
    }
}

/* Pull one exgmode_N line. Leave every other byte for the library.
 * A partial 'e' line is finished or dropped. It must not sit in the
 * UART buffer, or readString() takes the rest and the mode is lost.
 */
static void poll_mode_line(void)
{
    static uint8_t busy;

    if (busy) {
        return;
    }
    busy = 1;
    for (;;) {
        if (serial_waiting() == 0) {
            uint32_t t0;
            if (cmd_n == 0) {
                break;
            }
            t0 = millis();
            while (serial_waiting() == 0 && (uint32_t)(millis() - t0) < 20u) {
            }
            if (serial_waiting() == 0) {
                cmd_n = 0;
                break;
            }
        }
        {
            int c = Serial.peek();
            if (cmd_n == 0 && c != 'e') {
                break;
            }
            c = Serial.read();
            if (c < 0) {
                break;
            }
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                cmd[cmd_n] = 0;
                if (cmd_n == 9 && memcmp(cmd, "exgmode_", 8) == 0 &&
                    cmd[8] >= '0' && cmd[8] <= '2') {
                    reboot_into((uint8_t)(cmd[8] - '0'));
                }
                cmd_n = 0;
                break;
            }
            if (cmd_n < (uint8_t)(sizeof(cmd) - 1)) {
                cmd[cmd_n++] = (char)c;
            } else {
                cmd_n = 0;
            }
        }
    }
    busy = 0;
}

extern "C" int __wrap__ZN14HardwareSerial9availableEv(HardwareSerial *self)
{
    if (self == &Serial) {
        poll_mode_line();
    }
    return __real__ZN14HardwareSerial9availableEv(self);
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
