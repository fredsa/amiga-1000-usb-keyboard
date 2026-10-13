#include <EspUsbHost.h>
#include <freertos/queue.h>

#include "amiga.h"
#include "pc.h"
#include "test_keys.h"
#include "key_map.h"

#undef DEBUG_USB_DATA

#define KEYBOARD_REPORT_SIZE 8
#define KEYBOARD_REPORT_QUEUE_SIZE 20

// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0176.html
#define AMIGA_ACK_LOW_TIMEOUT_MS 145   // Slightly more than 143ms.
#define AMIGA_ACK_HIGH_TIMEOUT_MS 100  // Aribitrary.

// Amiga 1000 keyboard connector
// 1 Vcc (+5V)
// 2 PIN_AMIGA_KBCLK
// 3 PIN_AMIGA_KBDATA
// 4 GND

// ESP32S3 Dev Module
// Board URL: https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json
// - ESP32-S3-WROOM-1-N16R8
// - 16MB Quad SPI Flash
// - 8MB Octal SPI Flash
#define PIN_USB_DATA_MINUS 19  // ESP32-S3 hardwired
#define PIN_USB_DATA_PLUS 20   // ESP32-S3 hardwired
#define PIN_AMIGA_KBCLK 21     // Dealer's choice
#define PIN_AMIGA_KBDATA 47    // Dealer's choice
#define NEOPIXEL_NUM 1         // Onboard neo pixel


#ifdef NEOPIXEL_NUM
#define NEO_RED pixels.Color(10, 0, 0)
#define NEO_GREEN pixels.Color(0, 10, 0)
#define NEO_BLUE pixels.Color(0, 0, 10)
#define NEO_PURPLE pixels.Color(10, 0, 10)
#define NEO_YELLOW pixels.Color(10, 5, 0)
#define NEO_OFF pixels.Color(0, 0, 0)
#endif

extern EspUsbHost usb;
QueueHandle_t xQueue;

static bool syncd = false;
static bool capslock_on = false;
static bool ctrl_amiga_amiga = false;

// Keep track of which keys are pressed down.
static bool pc_down_keys[256];
const int pc_down_keys_len = sizeof(pc_down_keys) / sizeof(pc_down_keys[0]);

// USB HID supports up to 6 regular keys pressed simultaneously, plus 7 modifier keys we track.
static uint8_t usb_reported[] = { PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY,
                                  PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY };
const int usb_reported_len = sizeof(usb_reported) / sizeof(usb_reported[0]);
