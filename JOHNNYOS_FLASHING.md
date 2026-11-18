# JohnnyOS StickS3 flashing guide (preview v0.2)

**Target:** M5Stack StickS3, ESP32-S3-PICO-1-N8R8, 8 MB flash / 8 MB PSRAM.

## Flash the right file

The GitHub Actions artifact contains **one merged firmware image**:

`JohnnyOS-m5stack-sticks3.bin`

Use an ESP32-S3 flashing utility (ESP Web Tools/esptool/ESP flash programmer):

- Chip: **ESP32-S3**
- File: **JohnnyOS-m5stack-sticks3.bin**
- Start address / offset: **0x0000**
- Flash size: **8 MB**
- Flash mode / frequency: leave defaults/keep unless your tool requires a value
- Upload speed: start with **115200** if the connection has been unreliable

**Never write the merged image at 0x1000 or 0x10000.** Those offsets are for different image layouts. The merged artifact already includes the ESP32-S3 bootloader, partition table, and application.

Connect directly with a data-capable USB-C cable. If flashing reports serial corruption, retry at 115200, use a different cable/USB port and close other serial clients. Successful MD5 flash verification does not prove the application boots.

## Button navigation in current Bruce StickS3 port

The 3 physical button count includes the power/reset button; the Bruce input driver currently uses two menu buttons:

- GPIO 11: single click = **Next**; double click or hold = **Select**
- GPIO 12: single click = **Previous**; double click or hold = **Back**
- The third hardware power/reset button is **not** configured as an app navigation button.

Games reuse the two navigation buttons, with hold Back to exit. The final physical labeling needs device testing.

## Hardware test checklist

1. Flash the merged image at 0x0000.
2. Power cycle and look for the animated JohnnyOS splash screen.
3. Verify the custom first-page icons and selection behavior.
4. Open **Wi-Fi Scan** and verify real SSIDs / RSSI values.
5. Open **Games**, launch Snake/Pong/Reaction/Racing/Flappy, and exit.
6. Open **Themes**, change theme, power cycle and verify it persisted.
7. Open **System** for real battery, memory and flash readings.
8. Open the original Bruce tools (WiFi, BLE, IR, Files, Config).

If the screen is black after a verified flash, capture 115200 baud serial boot output. That is required before claiming hardware compatibility.

## Status

This is a **development preview**, not a production-tested v1.0 release.
The AI dashboard and microphone diagnostics exist, but **online AI conversations, offline speech recognition, AI voice responses, Bluetooth air mouse, and a dedicated phone web remote are not implemented**.
Do not mistake the AI orb for a working speech service.

Settings (themes, personality and game high scores) use NVS Preferences; they survive ordinary power loss and restart, but **an erase-all flash will wipe them**. Bruce's own settings remain managed by Bruce.

## Build from source

GitHub Actions workflow: `.github/workflows/johnnyos-sticks3.yml`
PlatformIO environment: `m5stack-sticks3`
Command: `pio run -e m5stack-sticks3`
Build script: `build.py`, merges bootloader at 0x0, partitions at 0x8000 and app at 0x10000.

See `JOHNNYOS_ROADMAP.md` for the staged feature and security plan.
