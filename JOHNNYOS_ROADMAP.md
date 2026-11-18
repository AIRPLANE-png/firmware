# JohnnyOS development plan (StickS3, Bruce fork)

JohnnyOS is a native ESP32-S3 firmware project built from Bruce; it is not a web page shown on the device. Its design target is a small iOS-inspired interface, with real functions and faithful capability labels.

## Hardware budget and safety

- Target board: M5Stack StickS3 / ESP32-S3-PICO-1-N8R8.
- Flash: 8 MB total; PSRAM: 8 MB total. The current `custom_8Mb.csv` factory app partition is 0x4E0000 (5,111,808 bytes, about 4.875 MiB), with a separate 0x300000 LittleFS data partition; monitor each compiler size.
- Native screen: 135 x 240; the Bruce environment rotates it to 240 x 135 landscape. Do not design 320 x 480 graphics or ship a browser/webview.
- The manufacturer lists 3 buttons including the power/reset key; the Bruce StickS3 port currently binds GPIO11 and GPIO12 to navigation. Do not reassign the power button without verifying power-management behavior.
- Avoid replacing existing Wi-Fi, BLE, IR, file, config, or other tools: call into the working Bruce menus.
- Favor small vector-style drawings, partial redraws, bounded task allocations, and PSRAM for large audio buffers.
- Never claim that successful compilation establishes boot/display reliability. Flash and hardware validation are separate gates.

## Stage 0: Working Bruce baseline (completed)

- Existing Bruce applications retained.
- Existing GitHub Actions StickS3 cloud build.
- Complete merged ESP32-S3 firmware image at flash address 0x0.
- Ability to recover with an unmodified Bruce or vendor image.

## Stage 1: Native JohnnyOS Preview v0.2 (source integrated, hardware test pending)

- Entry-point native launcher with distinct vector icons, gradient wallpaper, status information, selection ring, and paged navigation.
- Native animated boot splash.
- Native **Johnny AI** dashboard with animated orb, honest service status, Bruce microphone tools, Wi-Fi setup, and persisted personality selection.
- Native Wi-Fi scanner listing *real* nearby SSIDs, RSSI and channel details.
- Native games: Snake, Pong, Reaction, Mini Racing, Flappy. Two-button controls; high scores persisted for score-based games.
- Native Theme picker with Ocean, Violet and Mint; NVS preferences persist across power cycles.
- Native System view with battery, flash, PSRAM and available heap.
- Existing Bruce WiFi, BLE (including BLE scan/media), IR, Files, Config and other tools unchanged.
- CI validates that merged image exists, stays within total 8MB flash, and uploads a clearly named single firmware .bin.
- Verify stability and flash image on a real StickS3 before approving as hardware-tested.

## Stage 2: Visual polish + native UX

- Remove the remaining Bruce-style menus from JohnnyOS-first experiences while preserving their internal functions.
- Vector icon asset library, animated selected-state halo, optimized redraws, accessible text truncation.
- Add theme color mapping to all JohnnyOS pages, not only launcher/header.
- Real battery icon, correct clock source, connection status, network analyzer channel chart.
- Review pause and exit game behavior, continuous game frame times and watchdog health.
- Test brownout, abrupt power loss and persistence; store only *changes* to NVS to limit flash wear.
- Add build size and boot smoke-check to CI if a hardware runner is available.

## Stage 3: Online AI, microphone and speech

- AI network **backend required**. Do not hardcode a public API key into an ESP32 binary.
- Implement a user-owned HTTPS proxy protocol: POST audio/text, receive structured transcript, assistant response and optional audio stream.
- Use TLS certificate verification; reject arbitrary plaintext HTTP for private prompts.
- Store endpoint and optional tokens in a secure configuration strategy. Avoid secrets in GitHub Actions logs, source or downloadable firmware.
- Real push-to-talk: start/stop ES8311 microphone capture on the available GPIO/I2S codec path, upload PCM/Opus in bounded chunks.
- Transcribe and generate AI responses on the external backend (not on the ESP32-S3). Play streamed audio through the existing speaker codec.
- User-selectable AI personalities injected into backend system configuration.
- Offline voice commands: a tiny *verified embedded* keyword spotting/command model may be feasible for a short fixed set. General offline speech-to-text/LLM-quality conversations are **not** feasible at acceptable quality in 8 MB flash with Bruce resident. Prototype size/latency before promising.

## Stage 4: Remote controls, connectivity and files

- IR universal remote: keep Bruce transmit/receive code; add profile storage in LittleFS and a polished native handset interface.
- Bluetooth media remote: reuse Bruce BLE HID media commands, add status and keymaps.
- Bluetooth air mouse: implement BLE HID mouse report descriptor, IMU-based delta estimation, 2-button click/scroll mode, pairing and bond persistence. Check existing Bluetooth stack compatibility first.
- Phone web remote: an authenticated LAN HTTP/WebSocket server with a small phone UI to navigate device apps, issue allowed control actions and display connection status. Do not expose dangerous Bruce tools remotely without explicit safeguards.
- File manager: wrap existing Bruce filesystem access with browse/open/delete confirmation; don't store secrets in unprotected files.
- Wi-Fi settings and network information: reuse Bruce connect/credential management, not duplicate unsynchronized credential stores.
- Validate coexistence of Wi-Fi+BLE, battery life and PSRAM limits.

## Stage 5: Release criteria

- All promised features either genuinely work on hardware or are explicitly marked unavailable; never silently substitute animated demos.
- Tests: boots, navigation, screen orientation, sound/mic, BLE pairing, IR, FS, games, power loss, firmware reset, 8 MB flash budget and 8 MB PSRAM budget.
- Power-loss persistence: settings, high scores, network configuration and user files survive ordinary restarts; erase-all flashes and broken flash storage still require backup/restore.
- Signed tagged GitHub release with a validated merged flash image, version/changelog, SHA256, and fixed 0x0000 installation instructions.
- Build only through GitHub Actions `.github/workflows/johnnyos-sticks3.yml` using `pio run -e m5stack-sticks3`.

## Code map

- `src/core/main_menu.cpp/.h`: native launcher entry and Bruce app callback integration.
- `src/core/johnnyos/JohnnyApps.cpp/.h`: native AI dashboard, Wi-Fi scan, games, themes, system information.
- `src/main.cpp`: JohnnyOS splash screen on StickS3.
- `boards/m5stack-sticks3/interface.cpp`: GPIO, codec and power configuration.
- `boards/m5stack-sticks3/m5stack-sticks3.ini`: board-scoped JohnnyOS compile flag.
- `build.py`: merged complete `JohnnyOS-m5stack-sticks3.bin` for address 0x0000.
- `.github/workflows/johnnyos-sticks3.yml`: build and artifact validation.
- `JOHNNYOS_FLASHING.md`: flash/rollback guidance and hardware checklist.

**Rule:** Never mark a stage complete merely because a source file compiles. Report compiled, flashed, booted and hardware-tested separately.
