#include <ESP32-USB-Soft-Host.h>
#include <freertos/queue.h>

#include "USB.h"
#include "USBHIDKeyboard.h"

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

#ifdef NEOPIXEL_NUM
#define NEO_RED pixels.Color(20, 0, 0)
#define NEO_GREEN pixels.Color(0, 20, 0)
#define NEO_BLUE pixels.Color(0, 0, 20)
#define NEO_PURPLE pixels.Color(20, 0, 20)
#define NEO_YELLOW pixels.Color(20, 20, 0)
#define NEO_OFF pixels.Color(0, 0, 0)
#endif

#define FORCE_TEMPLATED_NOPS

// Amiga 1000 keyboard connector
// 1 Vcc (+5V)
// 2 PIN_AMIGA_KBCLK
// 3 PIN_AMIGA_KBDATA
// 4 GND

// Adafruit ESP32-S3 TFT Feather
// https://learn.adafruit.com/adafruit-esp32-s3-tft-feather
#define PIN_USB_DATA_MINUS A4  // USB Host Data- Pin (must be an analog pin)
#define PIN_USB_DATA_PLUS A5   // USB Host Data+ Pin (must be an analog pin)
#define PIN_AMIGA_KBCLK T10
#define PIN_AMIGA_KBDATA T11

// ** Have NOT gotten this to work on ESP32-S2! **
// Adafruit QT Py ESP32-S2
// https://learn.adafruit.com/adafruit-qt-py-esp32-s2
// #define PIN_USB_DATA_MINUS A2  // USB Host Data- Pin (must be an analog pin)
// #define PIN_USB_DATA_PLUS A3   // USB Host Data+ Pin (must be an analog pin)
// #define PIN_AMIGA_KBCLK SDA
// #define PIN_AMIGA_KBDATA SDC


const usb_pins_config_t USB_Pins_Config = {
  PIN_USB_DATA_PLUS, PIN_USB_DATA_MINUS,
  -1, -1,
  -1, -1,
  -1, -1
};

USBHIDKeyboard Keyboard;
QueueHandle_t xQueue;

static bool usb_dev_begun = false;
static bool syncd = false;
static bool capslock_on = false;
static bool ctrl_amiga_amiga = false;
static uint8_t keybd_dev_addr = 0xff;

// Keep track of which keys are pressed down.
static bool pc_down_keys[256];
const int pc_down_keys_len = sizeof(pc_down_keys) / sizeof(pc_down_keys[0]);

// USB HID supports up to 6 regular keys pressed simultaneously, plus 7 modifier keys we track.
static uint8_t usb_reported[] = { PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY,
                                  PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY, PC_KEYCODE_NOT_A_KEY };
const int usb_reported_len = sizeof(usb_reported) / sizeof(usb_reported[0]);
