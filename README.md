# Watch only hardware wallet

A quick demo what kind of custom bitcoin hardware you can do with [arduino-bitcoin](https://github.com/arduino-bitcoin/arduino-bitcoin) library.

## Why?

To show addresses safely without keeping your hardware wallet with you all the time.

Supports legacy addresses or legacy multisig.

## Hardware

Tried on the following hardware:

- [Arduino M0 Pro](https://store.arduino.cc/arduino-m0-pro)
- Adafruit 2.8 TFT screen with [capacitive](https://www.adafruit.com/product/1947) or [resistive](https://www.adafruit.com/product/1651) touch.

Should work on any 32-bit microcontroller and any TFT screen (maybe after some changes).

More docs will follow... at some point...

## Required libraries

Download and place to Arduino/libraries folder:

- [arduino-bitcoin](https://github.com/arduino-bitcoin/arduino-bitcoin)

Download via Library manager:

- Adafruit_GFX
- Adafruit_ILI9341
- Adafruit_FT6206 for capacitive touchscreen
- Adafruit_STMPE610 for resistive touchscreen
- QRCode library

## Roadmap

- [x] Support legacy addresses (P2PKH and P2SH multisig)
- [ ] Support nested segwit (P2SH-P2WPKH and P2SH-P2WSH)
- [ ] Support native bech32 segwit (P2WPKH and P2WSH)
- [ ] Load keys from SD card file
- [ ] Support more hardware boards and TFT screens
- [ ] Write better instructions
- [ ] Improve GUI