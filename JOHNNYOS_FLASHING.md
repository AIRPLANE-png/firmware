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

## Performance and battery (new v0.4 source)

JohnnyOS has three power profiles under **Themes / Appearance**. **KEY1 Next** changes the theme; **KEY1 Select** cycles the power profile; **KEY2 Back** exits.

| Power profile | Active backlight | Dim after inactivity |
| --- | --- | --- |
| Eco | 30% | 10 seconds |
| Balanced (default) | 58% | 20 seconds |
| Bright | 85% | 30 seconds |

After the standard dimmer period, Bruce's existing power-save path turns the screen off following its configured extra delay. A button press wakes it. Profiles are stored under the JohnnyOS NVS namespace and reapplied at boot without saving the Bruce configuration file on every boot.

Other optimizations: fewer full-screen redraws, shorter startup dot animation, faster two-button menu navigation, 60ms LCD wake delay, bounded game frame rates, lower idle IMU polling when the Bluetooth mouse is unpaired, and automatically turning off an otherwise unused Wi-Fi radio after JohnnyOS Wi-Fi scans.

**These are code-level optimizations, not verified battery-runtime measurements.** Battery savings depend on brightness, Wi-Fi/BLE use, peripherals, and your individual StickS3. Wi-Fi and Bluetooth remain available when their applications need them. No CPU overclock or untested radio clock changes have been added.

**Build status:** after any source change, use the latest successful GitHub Actions run for the matching commit SHA. An older artifact does *not* include these changes.

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

## JohnnyOS controls and new apps

The current Bruce StickS3 firmware uses two menu buttons (plus separate power/reset).

- GPIO11 / KEY1: tap **Next**, double-click or hold **Select**
- GPIO12 / KEY2: tap **Previous**, double-click or hold **Back**
- Open **Controls** from the launcher to review the button guide on the display.

### Phone web remote

Open **Phone Remote** and connect the StickS3 to Wi-Fi. It shows an 8-character access code and IP address ending in `:8181`. On your phone, join the same Wi-Fi network, open `http://<device-IP>:8181`, and type that code. You can then use Prev, Next, Select and Back from the phone. The server is local network only and does not expose unrelated tools through its HTTP API. Open Phone Remote again to choose **Stop phone remote**.

### Bluetooth air mouse and media remote

Open **Air Mouse**; pair your computer or phone with **JohnnyOS Mouse** using Bluetooth settings. Tilt the device to move the cursor. KEY1 tap = left click, KEY2 tap = right click, KEY2 hold = exit. This is a newly implemented Bluetooth HID mouse path and needs actual pairing and IMU testing. **Media** invokes Bruce's existing Bluetooth media-key application.

### Johnny AI voice recording

Open **Johnny AI** and select **Push-to-talk**. Hold KEY1 to speak and release to save a WAV file under `/johnnyos/last-voice.wav` on LittleFS. **Play last voice** uses the StickS3 speaker. This is genuine local recording/playback, **not cloud transcription or AI conversation yet**.

## Status

This is a **development preview**, not a production-tested v1.0 release.
Bluetooth HID mouse, phone remote, push-to-talk and games now have native source code, but require a successful complete build **and testing on the actual StickS3** before claiming they work.
**Online conversational AI, reliable offline speech recognition, AI-generated replies and voice responses** still require extra implementation and, for online AI, an external AI service.
Do not mistake the assistant orb for a working cloud AI service.

Settings (themes, personality and game high scores) use NVS Preferences; they survive ordinary power loss and restart, but **an erase-all flash will wipe them**. Bruce's own settings remain managed by Bruce.

## Build from source

GitHub Actions workflow: `.github/workflows/johnnyos-sticks3.yml`
PlatformIO environment: `m5stack-sticks3`
Command: `pio run -e m5stack-sticks3`
Build script: `build.py`, merges bootloader at 0x0, partitions at 0x8000 and app at 0x10000.

See `JOHNNYOS_ROADMAP.md` for the staged feature and security plan.
