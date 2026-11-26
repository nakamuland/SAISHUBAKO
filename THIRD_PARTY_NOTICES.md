# Third-party notices

SAISHUBAKO application code is provided under the MIT License in `LICENSE`.
The bundled firmware also incorporates libraries and fonts by other authors.
Their licenses and notices continue to apply.

| Component | Pinned version | Source and license information |
|---|---|---|
| M5Cardputer | 1.0.3 | https://github.com/m5stack/M5Cardputer — retain the copyright notices in its source |
| M5Unified | 0.2.29 | https://github.com/m5stack/M5Unified — MIT, see `licenses/M5Unified.txt` |
| M5GFX / bundled LovyanGFX code | 0.2.36 | https://github.com/m5stack/M5GFX — MIT and component notices, see `licenses/M5GFX.txt` |
| NimBLE-Arduino | 1.4.3 | https://github.com/h2zero/NimBLE-Arduino — Apache-2.0 and bundled component notices, see `licenses/NimBLE-Arduino.txt` |
| Arduino-IRremote | 4.7.1 | https://github.com/Arduino-IRremote/Arduino-IRremote — MIT, see `licenses/IRremote.txt` |
| /efont/ Japanese font | M5GFX distribution | Copyright 2000–2003 /efont/ The Electronic Font Open Laboratory and other font authors, see `licenses/efont.txt` |
| GFX fonts | M5GFX distribution | See `licenses/GFX-fonts.txt` |
| Arduino-ESP32 core | 2.0.16 | https://github.com/espressif/arduino-esp32/tree/2.0.16 — LGPL-2.1 and component notices |
| ESP-IDF and bootloader | supplied by Arduino-ESP32 2.0.16 | https://github.com/espressif/esp-idf — Apache-2.0 and component notices |

PlatformIO Espressif32 6.7.0 supplies the Arduino-ESP32 framework.
Building with the committed `platformio.ini` downloads the pinned dependency sources,
including their full component license and copyright notices.

## Printer protocol references

The printer driver implements the packet framing, checksum and command sequences
documented in public protocol research. These sources informed the implementation.

- MX11 command sequence and Python example: https://stackoverflow.com/questions/79301981/mx11-cat-thermal-printer-android-sdk
- FunPrint-compatible implementation: https://github.com/eraldonio/funprint
- MXW01 measurements and protocol notes: https://github.com/eliorpom-cmd/print-on-my-desk
- Cat/MX protocol notes: https://github.com/lobziik/jiji-purinto

The Fn+P entry point uses the `51 78` command sequence verified with the owner's MX11.
The alternative image-channel implementations in `src/mx11.h` remain experimental.
