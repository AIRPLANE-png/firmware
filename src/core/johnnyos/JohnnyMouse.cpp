#include "core/johnnyos/JohnnyExtras.h"
#if defined(JOHNNYOS_STICKS3)
#include "core/display.h"
#include "core/johnnyos/JohnnyApps.h"
#include "core/utils.h"
#include "modules/badusb_ble/ducky_typer.h"
#include "modules/ble/ble_common.h"
#include <M5Unified.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>
#include <NimBLEAdvertising.h>
#include <globals.h>

namespace {
constexpr uint8_t MOUSE_REPORT_ID = 3;
// HID report: buttons, signed X, signed Y, signed wheel.
const uint8_t mouseDescriptor[] = {
  0x05,0x01,0x09,0x02,0xA1,0x01,0x85,MOUSE_REPORT_ID,
  0x09,0x01,0xA1,0x00,
  0x05,0x09,0x19,0x01,0x29,0x03,0x15,0x00,0x25,0x01,
  0x95,0x03,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x05,0x81,0x03,
  0x05,0x01,0x09,0x30,0x09,0x31,0x09,0x38,0x15,0x81,0x25,0x7F,
  0x75,0x08,0x95,0x03,0x81,0x06,0xC0,0xC0
};
volatile bool mouseConnected = false;
class MouseCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override { mouseConnected = true; }
    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override { mouseConnected = false; }
};
MouseCallbacks mouseCallbacks;
void mouseScreen(bool connected, int dx, int dy) {
    johnnyDrawWallpaper();
    tft.setTextColor(tft.color565(255,255,255));
    tft.drawString("Air Mouse", 11, 7, 2);
    tft.setTextColor(tft.color565(161,207,255));
    tft.drawRightString(connected ? "CONNECTED" : "PAIRING", 230, 10, 1);
    tft.fillRoundRect(12, 29, 216, 82, 12, tft.color565(27,39,75));
    tft.drawCircle(52, 68, 24, tft.color565(70,130,230));
    tft.drawLine(52,48,52,88,tft.color565(165,218,255));
    tft.drawLine(32,68,72,68,tft.color565(165,218,255));
    tft.fillCircle(52,68,5,tft.color565(205,233,255));
    tft.setTextColor(tft.color565(255,255,255));
    tft.drawString(connected ? "Tilt to move" : "Pair: JohnnyOS Mouse", 92, 44, 1);
    tft.drawString(String("X ") + dx + "  Y " + dy, 92, 62, 1);
    tft.setTextColor(tft.color565(170,191,231));
    tft.drawString("NEXT: Left click", 92, 79, 1);
    tft.drawString("PREV: Right click", 92, 95, 1);
    tft.drawCentreString("Hold PREV: Back", 120, 121, 1);
}
void sendMouseReport(NimBLECharacteristic *report, uint8_t buttons, int dx, int dy, int wheel=0) {
    if (!report || !mouseConnected) return;
    const int8_t bytes[4] = {
       (int8_t)buttons, (int8_t)constrain(dx,-127,127),
       (int8_t)constrain(dy,-127,127), (int8_t)wheel
    };
    report->setValue(reinterpret_cast<const uint8_t *>(bytes), sizeof(bytes));
    report->notify();
}
} // namespace

void johnnyMouse() {
    // A BLE HID server cannot coexist with an unrelated, active Bruce BLE
    // service in the same controller. Ask user to disconnect first.
    if (BLEConnected || NimBLEDevice::getServer() != nullptr) {
        mouseScreen(false, 0, 0);
        tft.fillRoundRect(29, 52, 183, 29, 9, tft.color565(131,56,97));
        tft.setTextColor(TFT_WHITE);
        tft.drawCentreString("Disconnect other BLE first", 120, 61, 1);
        while (!check(EscPress)) delay(25);
        return;
    }
    NimBLEDevice::init("JohnnyOS Mouse");
    NimBLEDevice::setSecurityAuth(true, true, false);
    NimBLEServer *server = NimBLEDevice::createServer();
    server->setCallbacks(&mouseCallbacks, false);
    NimBLEHIDDevice *hid = new NimBLEHIDDevice(server);
    NimBLECharacteristic *report = hid->getInputReport(MOUSE_REPORT_ID);
    hid->setManufacturer("JohnnyOS");
    hid->setPnp(0x02, 0x303A, 0x4100, 0x0100);
    hid->setHidInfo(0x00, 0x01);
    hid->setReportMap((uint8_t *)mouseDescriptor, sizeof(mouseDescriptor));
    hid->startServices();
    hid->setBatteryLevel((uint8_t)getBattery());
    NimBLEAdvertising *advertising = server->getAdvertising();
    advertising->setAppearance(0x03C2);
    advertising->addServiceUUID(hid->getHidService()->getUUID());
    advertising->setName("JohnnyOS Mouse");
    advertising->start();

    check(NextPress); check(PrevPress); check(SelPress); check(EscPress);
    float zeroX = 0, zeroY = 0;
    M5.Imu.update();
    const auto sample = M5.Imu.getImuData();
    zeroX = sample.gyro.x;
    zeroY = sample.gyro.z;
    bool priorConnected = false;
    uint32_t lastDraw = 0;
    while (true) {
        if (check(EscPress)) break;
        bool leftClick = check(NextPress) || check(SelPress);
        bool rightClick = check(PrevPress);
        if (M5.Imu.update()) {
            const auto m = M5.Imu.getImuData();
            int dx = (int)((m.gyro.z - zeroY) * 0.065f);
            int dy = (int)((m.gyro.x - zeroX) * 0.065f);
            if (abs(dx) <= 1) dx = 0;
            if (abs(dy) <= 1) dy = 0;
            if (mouseConnected) {
                if (leftClick || rightClick) {
                    sendMouseReport(report, leftClick ? 1 : 2, 0, 0);
                    delay(35);
                    sendMouseReport(report, 0, 0, 0);
                } else if (dx || dy) sendMouseReport(report, 0, dx, dy);
            }
            if (lastDraw == 0 || priorConnected != mouseConnected) {
                mouseScreen(mouseConnected, dx, dy);
                lastDraw = millis();
            } else if (millis() - lastDraw > 300) {
                // Only update the small motion readout, not the entire screen.
                tft.fillRoundRect(91, 60, 125, 15, 4, tft.color565(27,39,75));
                tft.setTextColor(TFT_WHITE);
                tft.drawString(String("X ") + dx + "  Y " + dy, 92, 62, 1);
                lastDraw = millis();
            }
        }
        priorConnected = mouseConnected;
        delay(12);
    }
    advertising->stop();
    delete hid;
    NimBLEDevice::deinit(true);
    mouseConnected = false;
    check(NextPress); check(PrevPress); check(SelPress); check(EscPress);
}

void johnnyMediaRemote() {
#if !defined(LITE_VERSION)
    // Existing Bruce BLE HID implementation: no fake media buttons.
    MediaCommands(hid_ble, true);
#else
    displayInfo("Media remote requires full build", true);
#endif
}
#endif
