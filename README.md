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
2. From the IDE, install the support library for your particular dev board.
3. Update the `#define PIN_*` pin numbers in `amiga-1000-usb-keyboard.h`
   with any available pins on your dev board. Avoid pins on your devboard
   that are connected to onboard LEDs, are wired to other functions, or
   have pull up on pull down resistors hardwired on the board.
4. Optionally modify `key_map.h` to your desired keyboard confiuguraton.
4. Compile and push the build to the microcontroller.
5. Solder up the connections to your USB keyboard and telephone handset cord.

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

The number of pins that are broken out varies across different microcontroller dev boards.
The specific pins you will use depens on what you specified in `amiga-1000-usb-keyboard.h`.

## Wire connections

**WARNING**:

**Never** connect your microcontroller to your Amiga AND another computer at the same time.
Doing so can cause 5V power from one computer to backfeed into the other and damage it.
To avoid accidents, condier placing a piece of tape over the dev board's USB connector.

If you ever need to updated the software on the microcontroller:
1. UNPLUG the dev board from the Amiga 1000 Jack.
2. Only then plug your dev board into your computer's USB port.
3. Flash the microcontroller.
4. Unplug from the USB port.
5. Only then reattach to the Amiga.

Now that you've read the above warning, here are the connections you will need to make:

1. **Microcontroller `PIN_USB_DATA_MINUS`** to **USB keyboard Data -**
2. **Microcontroller `PIN_USB_DATA_PLUS`** to **USB keyboard Data +**
3. **Microcontroller `PIN_AMIGA_KBCLK`** to **Amiga Keyboard Clock**
4. **Microcontroller `PIN_AMIGA_KBDATA`** to **Amiga Keybaord Data**
5. **Microcontroller 5V** to **USB keyboard 5V** to **Amiga 5V**
   - If your dev board does not have a separate 5V pin, then use its USB connector's 5V pin.
6. **Microcontroller GND** to **USB keyboard GND** to **Amiga GND**
   - Note, your microcontroller's GND pin is likely connected to its USB GND pin.


