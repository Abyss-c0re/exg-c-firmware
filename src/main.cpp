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
#define EXG_FW_VERSION 4
#endif

static char cmd[12];
static uint8_t cmd_n;

extern "C" int __real__ZN14HardwareSerial9availableEv(HardwareSerial *self);

/* Bytes waiting on Serial. Calls the real available(), so the wrap does not re-enter. */
static int serial_waiting(void)
{
    return __real__ZN14HardwareSerial9availableEv(&Serial);
}

/* EEPROM byte 0. A value above 2, including a blank 0xFF, is mode 0. */
static uint8_t boot_mode(void)
{
    uint8_t m = EEPROM.read(0);
    if (m > 2) {
        return 0;
    }
    return m;
}

/* Starts the ADS1299 for mode 0, 1, or 2, then prints EXG-FW and EXG-MODE. */
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

/* Stores m, prints EXG-SWITCH, flushes, and hangs until the 15 ms watchdog resets. Does not return. */
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

/* Millis when a partial line started. 0 means there is no partial line. */
static uint32_t cmd_since;

/* Pull one exgmode_N line. Leave every other byte for the library.
 * A partial 'e' line is finished or dropped on a later call, 20 ms on.
 * available() does not wait. busy stops the wrap from entering twice. */
static void poll_mode_line(void)
{
    static uint8_t busy;

    if (busy) {
        return;
    }
    busy = 1;
    for (;;) {
        if (serial_waiting() == 0) {
            if (cmd_n == 0) {
                cmd_since = 0;
                break;
            }
            if (cmd_since == 0) {
                cmd_since = millis();
                if (cmd_since == 0) {
                    cmd_since = 1;
                }
                break;
            }
            if ((uint32_t)(millis() - cmd_since) < 20u) {
                break;
            }
            cmd_n = 0;
            cmd_since = 0;
            break;
        }
        cmd_since = 0;
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

/* On Serial, takes an exgmode_ line first, then returns the real available() count.
 * Any other UART is left alone. Do not call Serial.available() from here. */
extern "C" int __wrap__ZN14HardwareSerial9availableEv(HardwareSerial *self)
{
    if (self == &Serial) {
        poll_mode_line();
    }
    return __real__ZN14HardwareSerial9availableEv(self);
}

/* Reads the mode byte and starts the converter once. */
void setup(void)
{
    apply_mode(boot_mode());
}

/* Checks for exgmode_, then lets the library stream and read chon_ / rld lines. */
void loop(void)
{
    poll_mode_line();
    neuropawn.acquire_data();
}
