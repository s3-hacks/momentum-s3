# Momentum S3

**An unofficial port of the [Momentum Firmware](https://github.com/Next-Flip/Momentum-Firmware) look & feel for the M5Stack M5StickC S3 (M5StickS3).**

## ⚡ [Install in your browser → momentums3.netlify.app](https://momentums3.netlify.app)

No tools needed: open the link in **Chrome or Edge** on a computer, plug in the stick, click **Install**. You can back up your current firmware (e.g. Bruce) on the same page first.

Momentum is a custom firmware for the Flipper Zero. The Flipper uses an STM32 chip plus radio hardware (Sub-GHz, NFC, RFID) that the M5StickS3 does not have, so a 1:1 port is impossible. Momentum S3 instead rebuilds the Momentum experience on the ESP32-S3. Wherever possible it reuses Momentum's own assets and code: icons, dolphin animations, fonts, IR databases, universal remote layouts, the DuckyScript interpreter and the keyboard layouts.

> Not affiliated with or endorsed by the Momentum team or Flipper Devices.

## Features

| | |
|---|---|
| **Momentum UI** | DSi carousel or List main menu, original u8g2 fonts, status bar, all 35 dolphin animations with speech bubbles, level/XP system, passport, level-up animation, 10 color themes |
| **Infrared** | 9 universal remotes (TVs, ACs, Audio, Projectors, Fans, LEDs, Monitors, Blu-ray/DVD, Digital Signs) using Momentum's IR databases; learn remotes and save them in the Flipper `.ir` format |
| **Bad KB** | DuckyScript over **USB** or **Bluetooth**, ported from Momentum's `ducky_script`, with all Momentum keyboard layouts and demo scripts |
| **Apps** | Snake, Dice, Clock, Flashlight, WiFi scanner, BLE scanner, Web Files |
| **Web Files** | The stick opens its own WiFi. Upload `.ir` files and Bad KB scripts from your phone; the clock syncs automatically |

**Removed (hardware not present):** Sub-GHz, NFC, 125 kHz RFID, iButton, U2F, GPIO.

## Controls

| Button | Action |
|---|---|
| **M5 (front) – press** | OK / select |
| **Side – press** | Next |
| **Hold either button** | Back |

The power button is not used.

## Install

**Easiest:** use the **[web installer](https://momentums3.netlify.app)** (Chrome/Edge on a computer).

**Manual:**
1. Download `Momentum-S3-vX.X.bin` from the [Releases](../../releases) page.
2. Put the stick into download mode: hold the **side reset button ~2 s** until the green LED blinks.
3. Flash the file at address **0x0**, for example with esptool:
   ```
   esptool.py --chip esp32s3 write_flash 0x0 Momentum-S3-vX.X.bin
   ```

You can flash your previous firmware (e.g. Bruce) back the same way at any time.

### Web Files
Apps → Web Files, then connect your phone to WiFi **`Momentum-S3`** (password **`momentum123`**) and open **http://192.168.4.1**.

## Build from source

Requirements: [arduino-cli](https://arduino.github.io/arduino-cli/), ESP32 core **3.3.x**, libraries **M5Unified** and **IRremoteESP8266**.

```
arduino-cli core install esp32:esp32
arduino-cli lib install M5Unified IRremoteESP8266
arduino-cli compile --fqbn "esp32:esp32:esp32s3:USBMode=default,CDCOnBoot=default,FlashSize=8M,PSRAM=opi,PartitionScheme=default_8MB" --build-property "upload.maximum_size=6291456" --output-dir out firmware/MomentumS3
```

The flashable file is `out/MomentumS3.ino.merged.bin`. The first build takes a while because the animation data is large.

The `gen_*` files are generated from the Momentum source with `tools/gen_assets.py` (needs Python + Pillow):

```
python tools/gen_assets.py <path-to-Momentum-Firmware> firmware/MomentumS3
```

## Legal

Use Bad KB and IR only on your own devices or with permission.

## Credits & License

- **[Momentum Firmware](https://github.com/Next-Flip/Momentum-Firmware)** by the Next-Flip team: UI design, icons, dolphin animations, IR databases, universal remote layouts, DuckyScript code, keyboard layouts
- **[Flipper Zero firmware](https://github.com/flipperdevices/flipperzero-firmware)** by Flipper Devices
- [u8g2](https://github.com/olikraus/u8g2) fonts, [M5Unified/M5GFX](https://github.com/m5stack/M5Unified), [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266)

Licensed under the **GNU GPL v3.0** (see [LICENSE](LICENSE)), like Momentum itself.
