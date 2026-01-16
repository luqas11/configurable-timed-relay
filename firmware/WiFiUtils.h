#ifndef WiFiUtils_h
#define WiFiUtils_h

#include "Arduino.h"

// Sets up an AP using the given credentials, and attempts to connect to a WiFI network using the stored credentials, blinking the given LED while trying.
void beginWiFi(String ssidAP, String passwordAP, int ledPin);

// Checks if the WiFi connection is established and returns a boolean indicating it.
bool isWiFiConnected();

#endif