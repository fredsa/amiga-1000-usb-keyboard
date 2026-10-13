# Amiga 1000 USB keyboard adapter

Hook up any USB keyboard to your Amiga 1000.

<img src="working.png" width="300px">

## Hardware needed

1. Any USB keyboard.
   - A standard Windows keyboard will work.

2. An ESP32-S3 microcontroller dev board.
   - You can [get one](https://www.adafruit.com/search?q=esp32-s3) for under US $15.
   - Get one that has an onboard 5V -> 3.3V regulator. Most dev boards do.

4. An old telephone handset cord.
   - Or, any cord with a
     [4P4C](https://en.wikipedia.org/wiki/Modular_connector#4P4C) connector
     on one end, that will plug into the Amiga 1000 keyboard jack.
   - [RJ11](https://en.wikipedia.org/wiki/Registered_jack#RJ11) plugs
     will _not_ fit. They are too wide for the Amiga 1000 keyboard jack.

5. Soldering iron to make connections.


## Voltage considerations — 5V vs. 3.3V

The Amiga 1000 uses 5V logic whereas the ESP32-S3 uses 3.3V logic.
However, a (voltage) level shifter is not required.
- When the ESP32-S3 is provided with 3.3V (from the dev board's
  onboard 5V -> 3.3V regulator), it is tolerant of 5V on its data pins.
- 3.3V high and low signals are compatible with 5V logic. The Amiga will
  happily respond to 3.3V logic signals from the ESP32-S3.

The Amiga 1000 keyboard jack provides 5V power that can power the USB keyboard
and the ESP32-S3 dev board, through its onboard 5V -> 3.3V regulator.


## Getting started

1. Install the [Arduino IDE](https://www.arduino.cc/en/software/).
2. Install the `esp32` board platform by Espressif Systems (version 3.x) via the Boards Manager.
3. If required for your board, add its Additional boards manager URL in
   Preferences.
4. Install the **EspUsbHost** library by TANAKA Masayuki via **Sketch > Include Library > Manage Libraries...**.
5. Configure Arduino IDE **Tools** settings so that USB-OTG is not used by
   settings:
   - **Board**: select your board
   - **USB Mode**: `USB-OTG (TinyUSB)`
   - **USB CDC On Boot**: `Disabled`
   - **Upload Mode**: `UART0 / Hardware CDC`
6. Update the pin assignments in `amiga-1000-usb-keyboard.h`:
   - **Amiga KBCLK**: `PIN_AMIGA_KBCLK`
   - **Amiga KBDATA**: `PIN_AMIGA_KBDATA`
7. Optionally modify `key_map.h` to your desired keyboard configuration.
8. Compile and push the build to the microcontroller.
9.  Connect your USB keyboard and solder up the connections to your telephone
    handset cord.

## Pinouts

### Amiga 1000 keyboard connector
- PIN 1 Vcc (+5V)
- PIN 2 Keyboard Clock
- PIN 3 Keyboard Data
- PIN 4 GND

### USB keyboard connector
- PIN 1 USB Vcc (+5V)
- PIN 2 USB Data -
- PIN 3 USB Data +
- PIN 4 USB GND

### ESP32-S3 dev board

ESP32-S3 native USB OTG Host is hardwired to the internal PHY:
- **GPIO 19**: USB Host Data -
- **GPIO 20**: USB Host Data +

You can either wire the USB keyboard lines to GPIO 19 and GPIO 20, or simply plug the keyboard into the board's native USB-C port labeled **USB** using a USB-C to USB-A OTG adapter.

## Wire connections

**WARNING**:

**Never** connect your microcontroller to your Amiga AND another computer at the
same time. Doing so can cause 5V power from one computer to backfeed into the
other and damage it. To avoid accidents, consider placing a piece of tape over
the dev board's UART USB connector when attached to the Amiga.

If you ever need to update the software on the microcontroller:
1. UNPLUG the dev board from the Amiga 1000 Jack.
2. Only then plug your dev board into your computer's USB port.
3. Flash the microcontroller.
4. Unplug from the USB port.
5. Only then reattach to the Amiga.

Now that you've read the above warning, here are the connections you will need to make:

1. **Microcontroller `PIN_USB_DATA_MINUS`** to **USB keyboard Data -**
   (or use onboard USB OTG port)
3. **Microcontroller `PIN_USB_DATA_PLUS`** to **USB keyboard Data +**
   (or use onboard USB OTG port)
4. **Microcontroller `PIN_AMIGA_KBCLK`** to **Amiga Keyboard Clock**
5. **Microcontroller `PIN_AMIGA_KBDATA`** to **Amiga Keyboard Data**
6. **Microcontroller 5V** to **USB keyboard 5V** to **Amiga 5V**
   - If your dev board does not have a separate 5V pin, use its USB connector's
     5V pin.
7. **Microcontroller GND** to **USB keyboard GND** to **Amiga GND**


