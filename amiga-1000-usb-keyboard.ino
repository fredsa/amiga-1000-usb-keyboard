#ifdef NEOPIXEL_NUM
#include <Adafruit_NeoPixel.h>
#endif

#include "amiga-1000-usb-keyboard.h"

// https://github.com/tobozo/ESP32-USB-Soft-Host compatibility:
// - Compiles with `esp32` Espressif Systems library version 2.0.17
// - Will not compile version 3.3.3

#if !defined CONFIG_IDF_TARGET_ESP32S3 && !defined CONFIG_IDF_TARGET_ESP32S2
#error "This sketch will only run on ESP32-S2 or ESP32-S3"
#endif

#if ARDUINO_USB_MODE
#error "Please set: Tools > Setttings > USB Mode: USB-OTG (TinyUSB)"
#endif

#ifdef NEOPIXEL_NUM
Adafruit_NeoPixel pixels(NEOPIXEL_NUM, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
#endif

void setup() {
  // Blink main LED.
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

#ifdef NEOPIXEL_NUM
  // Configure neopixel.
  pixels.begin();
  neo_color(NEO_PURPLE);
#endif

  // Configure UART serial.
  // Match ESP32-S3 ROM boot messages: 115200 8N1
  Serial.begin(115200);

  Serial.println("[AmigaUSB] Settting up...");
  Serial.println();

  // Configure keyboard.
  init_keycode_map();
  for (int i = 0; i < pc_down_keys_len; i++) {
    pc_down_keys[i] = false;
  }

  // Configure queue to buffer inter-process communication.
  xQueue = xQueueCreate(KEYBOARD_REPORT_QUEUE_SIZE, sizeof(uint8_t) * KEYBOARD_REPORT_SIZE);

  // Configure digital pins.
  pinMode(PIN_AMIGA_KBCLK, OUTPUT);
  pinMode(PIN_AMIGA_KBDATA, OUTPUT);
  digitalWrite(PIN_AMIGA_KBCLK, HIGH);
  digitalWrite(PIN_AMIGA_KBDATA, HIGH);

  // Configure USB host.
  USH.setOnConfigDescCB(Default_USB_ConfigDescCB);
  USH.setOnIfaceDescCb(Default_USB_IfaceDescCb);
  USH.setOnHIDDevDescCb(Default_USB_HIDDevDescCb);
  USH.setOnEPDescCb(Default_USB_EPDescCb);

  USH.init(USB_Pins_Config, on_usb_keyboard_detect, on_usb_keyboard_data);

  // Setup complete.
  Serial.println("[AmigaUSB] Setup complete.");
  Serial.flush();

#ifdef NEOPIXEL_NUM
  neo_color(NEO_YELLOW);
#endif
}

void loop() {
  process_usb_keyboard_data();
}

#ifdef NEOPIXEL_NUM
void neo_color(uint32_t color) {
  pixels.setPixelColor(0, color);
  pixels.show();
}
#endif

bool amiga_sync_up() {
  if (syncd) {
    return true;
  }

  print_uptime();
  Serial.println("[AmigaUSB]");
  Serial.println("[AmigaUSB] Syncing...");

  // Limited attempts, so hard reset is checked regardless of sync state.
  unsigned long int count = 0;
  for (int i = 0; i < 10; i++) {
    count++;
    pinMode(PIN_AMIGA_KBDATA, OUTPUT);
    send_bit(1);

    wait_for_amiga_ack(true);
    if (syncd) {
      Serial.printf("[AmigaUSB] ACK received after %lu attempts.\n", count);
      return true;
    }
  }

  Serial.printf("[AmigaUSB] Still NO ack after %lu attempts.\n", count);
  return false;
}

void wait_for_amiga_ack(bool debug) {
  pinMode(PIN_AMIGA_KBDATA, INPUT);
  int lowafter = wait_for_state(PIN_AMIGA_KBDATA, LOW, AMIGA_ACK_LOW_TIMEOUT_MS * 1000);
  if (lowafter != -1) {
    int highafter = wait_for_state(PIN_AMIGA_KBDATA, HIGH, AMIGA_ACK_HIGH_TIMEOUT_MS * 1000);
    if (highafter != -1) {
      if (debug) {
        Serial.printf("[AmigaUSB] Got sync LOW after %i us, HIGH after %i us.\n", lowafter, highafter);
      }
      set_syncd(true);
      return;
    }
  }
  set_syncd(false);
  return;
}

int wait_for_state(uint8_t pin, uint8_t desired, int timeout_us) {
  unsigned long start = micros();
  unsigned long until = micros() + timeout_us;
  while (true) {
    unsigned long now = micros();
    if (now > until) {
      return -1;
    }
    uint8_t state = digitalRead(pin);
    if (state == desired) {
      return now - start;
    }
  }
}

void print_uptime() {
  unsigned long currentMillis = millis();

  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  Serial.printf("[AmigaUSB] Uptime: %02lu:%02lu:%02lu", hours, minutes, seconds);
}

// Amiga Test Kit 1.21
// https://github.com/keirf/amiga-stuff
void do_amiga_test_kit_keyboard_test() {
  // Exit back to main menu.
  send_amiga_keycode(AMIGA_KEYCODE_CTRL);
  send_amiga_keycode(AMIGA_KEYCODE_LEFT_ALT);
  delay(500);

  // Enter Amiga Test Kit 1.21 keyboard test.
  send_amiga_keycode_up_down(AMIGA_KEYCODE_F2);
  delay(1000);

  // Send test keys.
  size_t array_size = sizeof(test_keys) / sizeof(test_keys[0]);
  for (int i = 0; i < array_size; i++) {
    if (!syncd) {
      // Abort.
      return;
    }
    send_amiga_keycode_up_down(test_keys[i]);
  }

  delay(3000);

  // Exit back to main menu.
  send_amiga_keycode(AMIGA_KEYCODE_CTRL);
  send_amiga_keycode(AMIGA_KEYCODE_LEFT_ALT);
  delay(500);
}

// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0173.html
void send_bit(uint8_t bit) {
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(PIN_AMIGA_KBDATA, OUTPUT);
  digitalWrite(PIN_AMIGA_KBDATA, !bit);  // Active low.
  delayMicroseconds(20);
  digitalWrite(PIN_AMIGA_KBCLK, LOW);
  delayMicroseconds(20);
  digitalWrite(PIN_AMIGA_KBCLK, HIGH);
  delayMicroseconds(20);
  digitalWrite(LED_BUILTIN, LOW);
}

void send_amiga_keycode_up_down(uint8_t amiga_keycode) {
  send_amiga_keycode(amiga_keycode | AMIGA_KEYCODE_BITMASK_PRESS);
  delay(300);
  send_amiga_keycode(amiga_keycode | AMIGA_KEYCODE_BITMASK_RELEASE);
}

void send_amiga_keycode(uint8_t amiga_keycode) {
  if (!amiga_sync_up()) {
    Serial.printf("[AmigaUSB] ERROR: Failed to sync, dropping Amiga Keycode 0x%02X!\n", amiga_keycode);
    return;
  }

#ifdef NEOPIXEL_NUM
  neo_color(NEO_BLUE);
#endif
  for (int i = 6; i >= 0; i--) {
    uint8_t bit = (amiga_keycode >> i) & 0x01;
    send_bit(bit);
  }

  uint8_t bit = amiga_keycode >> 7;
  send_bit(bit);

  digitalWrite(PIN_AMIGA_KBDATA, HIGH);

  wait_for_amiga_ack(false);
  if (syncd) {
    Serial.printf("[AmigaUSB] Sent Amiga 0x%02X, received ACK.\n", amiga_keycode);
    return;
  } else {
    Serial.printf("[AmigaUSB] Sent Amiga 0x%02X, NO ACK! RETRYING...\n", amiga_keycode);
  }
}

void set_syncd(bool state) {
  syncd = state;
#ifdef NEOPIXEL_NUM
  if (syncd) {
    neo_color(NEO_OFF);
  } else {
    neo_color(NEO_RED);
  }
#endif
}

static void on_usb_keyboard_detect(uint8_t usbNum, void* dev) {
  sDevDesc* device = (sDevDesc*)dev;
  Serial.printf("[AmigaUSB] New device detected on USB#%d\n", usbNum);
  Serial.printf("[AmigaUSB] desc.bcdUSB             = 0x%04x\n", device->bcdUSB);
  Serial.printf("[AmigaUSB] desc.bDeviceClass       = 0x%02X\n", device->bDeviceClass);
  Serial.printf("[AmigaUSB] desc.bDeviceSubClass    = 0x%02X\n", device->bDeviceSubClass);
  Serial.printf("[AmigaUSB] desc.bDeviceProtocol    = 0x%02X\n", device->bDeviceProtocol);
  Serial.printf("[AmigaUSB] desc.bMaxPacketSize0    = 0x%02X\n", device->bMaxPacketSize0);
  Serial.printf("[AmigaUSB] desc.idVendor           = 0x%04x\n", device->idVendor);
  Serial.printf("[AmigaUSB] desc.idProduct          = 0x%04x\n", device->idProduct);
  Serial.printf("[AmigaUSB] desc.bcdDevice          = 0x%04x\n", device->bcdDevice);
  Serial.printf("[AmigaUSB] desc.iManufacturer      = 0x%02X\n", device->iManufacturer);
  Serial.printf("[AmigaUSB] desc.iProduct           = 0x%02X\n", device->iProduct);
  Serial.printf("[AmigaUSB] desc.iSerialNumber      = 0x%02X\n", device->iSerialNumber);
  Serial.printf("[AmigaUSB] desc.bNumConfigurations = 0x%02X\n", device->bNumConfigurations);

  if (!usb_dev_begun) {
    Serial.println("[AmigaUSB] Starting USB");
    Keyboard.begin();
    USB.begin();
  }
  keybd_dev_addr = usbNum;
}

static void on_usb_keyboard_data(uint8_t usbNum, uint8_t byte_depth, uint8_t* data, uint8_t data_len) {
  // Forward data to microcontroller USB bus.
  Keyboard.sendReport((KeyReport*)data);

  if (data_len != KEYBOARD_REPORT_SIZE) {
    Serial.printf("[AmigaUSB] ERROR: Received keyboard report of unexpected length: %i\n", data_len);
    for (int k = 0; k < data_len; k++) {
      if (k == 2) { Serial.printf("[AmigaUSB]  "); }
      Serial.printf("[AmigaUSB] 0x%02X ", data[k]);
    }
    Serial.println("[AmigaUSB]");
    return;
  }

  xQueueSend(xQueue, data, 0);
}

static void process_usb_keyboard_data() {
  uint8_t data[KEYBOARD_REPORT_SIZE];
  if (xQueueReceive(xQueue, &data, 0) != pdTRUE) {
    return;
  }

#ifdef DEBUG_USB_DATA
  Serial.printf("[AmigaUSB] process_usb_keyboard_data: ");
  for (int k = 0; k < KEYBOARD_REPORT_SIZE; k++) {
    if (k == 2) { Serial.printf("[AmigaUSB]  "); }
    Serial.printf("[AmigaUSB] 0x%02X ", data[k]);
  }
  Serial.println("[AmigaUSB]");
#endif

  if (data[2] == PC_KEYCODE_TOO_MANY_KEYS && data[3] == PC_KEYCODE_TOO_MANY_KEYS && data[4] == PC_KEYCODE_TOO_MANY_KEYS && data[5] == PC_KEYCODE_TOO_MANY_KEYS && data[6] == PC_KEYCODE_TOO_MANY_KEYS && data[7] == PC_KEYCODE_TOO_MANY_KEYS) {
    Serial.printf("[AmigaUSB] ERROR: Keyboard reports too many keys pressed\n");
    return;
  }

  // Populate fake PC key codes for modifier keys.
  usb_reported[0] = (data[0] & ((1 << 0) | (1 << 4))) != 0 ? PC_KEYCODE_FAKE_CTRL : PC_KEYCODE_NOT_A_KEY;
  usb_reported[1] = (data[0] & (1 << 1)) != 0 ? PC_KEYCODE_FAKE_LEFT_SHIFT : PC_KEYCODE_NOT_A_KEY;
  usb_reported[2] = (data[0] & (1 << 2)) != 0 ? PC_KEYCODE_FAKE_LEFT_ALT : PC_KEYCODE_NOT_A_KEY;
  usb_reported[3] = (data[0] & (1 << 3)) != 0 ? PC_KEYCODE_FAKE_LEFT_SUPER : PC_KEYCODE_NOT_A_KEY;
  usb_reported[4] = (data[0] & (1 << 5)) != 0 ? PC_KEYCODE_FAKE_RIGHT_SHIFT : PC_KEYCODE_NOT_A_KEY;
  usb_reported[5] = (data[0] & (1 << 6)) != 0 ? PC_KEYCODE_FAKE_RIGHT_ALT : PC_KEYCODE_NOT_A_KEY;
  usb_reported[6] = (data[0] & (1 << 7)) != 0 ? PC_KEYCODE_FAKE_RIGHT_SUPER : PC_KEYCODE_NOT_A_KEY;
  // Copy over non-modfier key codes.
  for (int i = 0; i < KEYBOARD_REPORT_SIZE - 2; i++) {
    usb_reported[7 + i] = data[2 + i];
  }

  // Check for CTRL-SUPER-SUPER.
  // http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0178.html
  bool reset_keys = usb_reported[0] != PC_KEYCODE_NOT_A_KEY && usb_reported[3] != PC_KEYCODE_NOT_A_KEY && usb_reported[6] != PC_KEYCODE_NOT_A_KEY;
  if (reset_keys | ctrl_amiga_amiga) {
    ctrl_amiga_amiga = reset_keys;
    send_amiga_hard_reset(ctrl_amiga_amiga);
  }

  // Don't send key data while reset keys are being held.
  if (ctrl_amiga_amiga) {
    return;
  }

#ifdef DEBUG_USB_DATA
  Serial.printf("[AmigaUSB] << known pc_down_keys: ");
  for (int i = 0; i < pc_down_keys_len; i++) {
    if (pc_down_keys[i]) {
      Serial.printf("[AmigaUSB] 0x%02X ", i);
    }
  }
  Serial.printf("[AmigaUSB]    usb_reported: ");
  for (int i = 0; i < usb_reported_len; i++) {
    Serial.printf("[AmigaUSB] 0x%02X ", usb_reported[i]);
  }
  Serial.println("[AmigaUSB]");
#endif

  // Process newly released keys.
  for (int i = 0; i < pc_down_keys_len; i++) {
    if (i == PC_KEYCODE_NOT_A_KEY) {
      continue;
    }
    bool found = false;
    if (pc_down_keys[i]) {
      for (int k = 0; k < usb_reported_len; k++) {
        if (usb_reported[k] == i) {
          found = true;
          break;
        }
      }
      if (!found) {
        pc_down_keys[i] = false;
        send_pc_key_state_change(i, AMIGA_KEYCODE_BITMASK_RELEASE);
      }
    }
  }

  // Process newly pressed keys.
  for (int i = 0; i < usb_reported_len; i++) {
    uint8_t pc_keycode = usb_reported[i];
    if (pc_keycode == PC_KEYCODE_NOT_A_KEY) {
      continue;
    }
    if (!pc_down_keys[pc_keycode]) {
      pc_down_keys[pc_keycode] = true;
      send_pc_key_state_change(pc_keycode, AMIGA_KEYCODE_BITMASK_PRESS);
    }
  }

#ifdef DEBUG_USB_DATA
  Serial.printf("[AmigaUSB] >> known pc_down_keys: ");
  for (int i = 0; i < pc_down_keys_len; i++) {
    if (pc_down_keys[i]) {
      Serial.printf("[AmigaUSB] 0x%02X ", i);
    }
  }
  Serial.println("[AmigaUSB]");
  Serial.println("[AmigaUSB]");
#endif
}

void send_pc_key_state_change(uint8_t pc_code, uint8_t amiga_down_up_mask) {
  if (pc_code == AMIGA_KEYCODE_NOT_A_KEY) {
    return;
  }
  uint8_t amiga_code = keycode_map[pc_code];
  if (amiga_code == AMIGA_KEYCODE_NOT_A_KEY) {
    return;
  }

  // http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0175.html
  if (amiga_code == AMIGA_KEYCODE_CAPS_LOCK) {
    if (amiga_down_up_mask) {
      return;
    }
    capslock_on = !capslock_on;
    send_amiga_keycode(amiga_code | (capslock_on ? AMIGA_KEYCODE_BITMASK_PRESS : AMIGA_KEYCODE_BITMASK_RELEASE));
    return;
  }

  send_amiga_keycode(amiga_code | amiga_down_up_mask);
}

// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0179.html
void send_amiga_hard_reset(bool reset_keys_held) {
  if (reset_keys_held) {
    Serial.printf("[AmigaUSB] CTRL-Amiga-Amiga held, initiating hard reset...\n");
  } else {
    Serial.printf("[AmigaUSB] CTRL-Amiga-Amiga released.\n");
  }
  digitalWrite(PIN_AMIGA_KBCLK, reset_keys_held ? LOW : HIGH);
}