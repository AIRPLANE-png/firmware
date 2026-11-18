#pragma once

#if defined(JOHNNYOS_STICKS3)
#include <Arduino.h>

// These apps are native StickS3 features, not web mockups.
void johnnyPhoneRemote();
void johnnyMouse();
void johnnyMediaRemote();
void johnnyPushToTalk();
void johnnyPlayLastVoice();

// Pump a single authorized phone command into Bruce's existing navigation
// flow, from the loop task rather than from an AsyncWebServer worker task.
bool johnnyRemotePop(uint8_t &key);
#endif
