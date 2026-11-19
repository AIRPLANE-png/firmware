# JohnnyOS StickS3 flashing guide (Visual Edition v0.6)

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

- GPIO 11: single click = **Next** by default; double click or hold = **Select** (always fixed)
- GPIO 12: single click = **Previous** by default; double click or hold = **Back** (always fixed)
- Tap directions can be reversed in **Controls**, page 6, or in the paired browser control center.
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

The JohnnyOS StickS3 firmware uses two menu buttons (plus separate power/reset).

- GPIO11 / KEY1: tap **Next** (or Previous when swapped), double-click or hold **Select**
- GPIO12 / KEY2: tap **Previous** (or Next when swapped), double-click or hold **Back**
- Open **Controls**, page 6, to switch tap directions on-device; the selection and Back holds are never remapped.

### Phone web remote

Open **Phone Remote** and connect the StickS3 to Wi-Fi. It shows an 8-character access code and IP address ending in `:8181`. On your phone, join the same Wi-Fi network, open `http://<device-IP>:8181`, and type that code. You can then use Prev, Next, Select and Back from the phone, choose a theme and power profile, reverse tap directions, and ask the local offline assistant questions. The server uses a code shown on the device, never embedded in the page or sent through the URL. It is **plain HTTP** intended for a trusted local Wi-Fi network; do **not** forward port 8181 or expose it to the internet. It does not expose unrelated Bruce tools through its API. Open Phone Remote again to choose **Stop phone remote**.

### Bluetooth air mouse and media remote

Open **Air Mouse**; pair your computer or phone with **JohnnyOS Mouse** using Bluetooth settings. Tilt the device to move the cursor. KEY1 tap = left click, KEY2 tap = right click, KEY2 hold = exit. This is a newly implemented Bluetooth HID mouse path and needs actual pairing and IMU testing. **Media** invokes Bruce's existing Bluetooth media-key application.

### Johnny AI voice recording

Open **Johnny AI** and select **Record voice**. Hold KEY1 to speak and release to save a WAV file under `/johnnyos/last-voice.wav` on LittleFS. **Play voice** uses the StickS3 speaker. This is genuine local recording/playback, **not cloud transcription or AI conversation yet**.

### JohnnyOS offline assistant (v0.5 source)

From **Johnny AI → Offline Q&A**, use short taps to browse ten built-in topics and hold KEY1 to display a response. This feature runs **on the StickS3 without Wi-Fi or internet**. Topics include battery, Wi-Fi, controls, files, games, power saving, Bluetooth, the browser remote and recorded voice.

The browser's **Offline assistant** section accepts typed questions over the local Wi-Fi connection and uses the same **on-device keyword/intent engine**. It makes no cloud requests. The browser is optional, and needs local Wi-Fi solely to reach the StickS3.

**Important:** This is an offline, deterministic question-answering assistant, **not an offline large language model**, free-form on-device speech recognition, or AI-generated speech. Unknown questions receive a fallback message. The on-device version offers ten selectable prompts because the StickS3 has no text keyboard. The web companion permits text input.

### Visual and controls refresh (v0.5 source)

The home screen paints 30×30 app icons centered inside three-column rounded cards; selection changes repaint only affected cards. The two-button driver responds to click events without the old 200 ms polling gate. The browser controller can reverse short-tap direction (Next/Previous), change the theme and select Eco/Balanced/Bright; settings are validated and applied from the menu task, then saved in NVS. Long/double-tap actions retain Select and Back for safe recovery.

GitHub Actions is configured for **manual workflow_dispatch** builds only to avoid the previous flood of cancelled build jobs. The latest successful build must match the commit that contains these source changes. Do not flash an older v0.4 artifact expecting v0.5 features.

## Visual Edition v0.6 (new source, hardware testing pending)

The launcher supports **Landscape (240x135)** and **Portrait (135x240)**. Go to **Settings -> Orientation** and select the other mode. **Only the home launcher rotates**; all Bruce and JohnnyOS apps open in landscape because their controls/layouts were designed for 240x135. The browser control center also has an Orientation selector. Back and Select are never remapped.

The launcher uses opaque rounded icon tiles with selective redraw, pinned favorite text in its bottom dock, a battery/Wi-Fi/sound header, and three color themes with three wallpaper styles: Gradient, Midnight and Aurora. The dock is a **favorite indicator**, not a third hardware navigation button; pinning moves your favorite to the first home tile for quick access.

**Home Layout** lets you pin a favorite and move apps earlier/later. These preferences are stored locally. The menu preserves all original Bruce tools, including the original Files app.

New native apps:

- **Calculator:** 4-column keypad, basic arithmetic, decimals, delete, sign and clear. Tap to choose a key, hold Key 1 to press.
- **Files +:** safer read-only LittleFS root explorer and text preview. It does not edit/delete files or replace the original Bruce SD file manager.
- **Performance:** battery, free heap, free PSRAM and Wi-Fi connection, updated approximately once per second.
- **2048 Puzzle:** select direction with taps, hold Key 1 to slide.
- **Memory Duel:** repeat the displayed two-button color sequence.

**Audio is OFF by default.** Change it in **Johnny AI -> Audio** or **Settings -> Audio / volume**. Disabling audio blocks JohnnyOS voice recording playback and the legacy Bruce boot jingle; voice recording remains available. Other legacy tools may have their own separate audio settings.

**Experimental offline voice activity:** Johnny AI -> Voice activity records a WAV through the existing mic support and checks its audio amplitude. It can report noisy/quiet audio. This **does not recognize words, perform speech transcription, run a local language model or provide spoken AI answers**. The existing Offline Q&A remains a deterministic device-help assistant; more topics cover Calculator, 2048, Portrait, audio, favorites and performance.

**Phone Control Center:** open Phone Remote, view the IP/code, then visit the displayed device URL while your phone and StickS3 share a trusted Wi-Fi network. The web interface now includes gamepad buttons, live cached battery/free-heap/Wi-Fi telemetry, theme/wallpaper/orientation/power/sound controls, and the offline question box. The page has home-screen app metadata and a local manifest, but because a local IP normally serves HTTP without HTTPS, **full PWA installation/service-worker caching may not work** in some browsers. Use your browser's **Add to Home Screen** for a shortcut. **Do not expose port 8181 to the internet**; the local pairing code is not end-to-end encrypted over HTTP.

### Reliability and compatibility checklist

1. Confirm a green successful GitHub Actions build for the **v0.6 commit**, and use its matching **JohnnyOS-StickS3-Visual-v0.6** artifact, not old v0.5.
2. Flash the merged image at **0x0000**; verify the screen and both buttons with hardware.
3. Toggle portrait in Settings and confirm the launcher rotates, while existing tools return to landscape.
4. Move Home Layout icons and pin a favorite, then restart and verify persistence.
5. Confirm that sounds remain muted by default and can be enabled through Settings or Johnny AI.
6. Test the Calculator operations, 2048 moves, Memory Duel, Files + preview, and performance readings.
7. Pair the local browser, navigate the home menu, use the gamepad in a game and save preferences.
8. Record voice activity through the mic, and **do not** interpret activity detection as speech recognition.
9. Measure battery and responsiveness on physical hardware; code-level optimization does not prove a particular speed or battery improvement.

## Status

This is a **development preview**, not a production-tested v1.0 release.
Bluetooth HID mouse, phone remote, push-to-talk and games now have native source code, but require a successful complete build **and testing on the actual StickS3** before claiming they work.
**Online conversational AI, offline large-language-model chat, reliable offline speech recognition, AI-generated replies and voice responses** still require extra implementation and, for online AI, an external AI service.
Do not mistake the assistant orb for a working cloud AI service.

Settings (themes, personality and game high scores) use NVS Preferences; they survive ordinary power loss and restart, but **an erase-all flash will wipe them**. Bruce's own settings remain managed by Bruce.

## Build from source

GitHub Actions workflow: `.github/workflows/johnnyos-sticks3.yml`
PlatformIO environment: `m5stack-sticks3`
Command: `pio run -e m5stack-sticks3`
Build script: `build.py`, merges bootloader at 0x0, partitions at 0x8000 and app at 0x10000.

See `JOHNNYOS_ROADMAP.md` for the staged feature and security plan.
