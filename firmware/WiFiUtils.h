#ifndef WiFiUtils_h
#define WiFiUtils_h

#include "Arduino.h"

// Attempts to connect the system to a WiFi network with the given credentials and initializes the access point network. Retries indefinitely until a connection is successfully established. Blinks a given LED while trying, and turns it on if success.
void connectToWifi(String ssid, String password, String ssidAP, String passwordAP, int ledPin);

// Checks if the WiFi connection is established and returns a boolean indicating it.
bool isWiFiConnected();

// Initializes the Time library.
void beginTime();

// Checks if the system hour is set and returns a boolean indicating it.
bool isTimeSet();

// Gets the current system hour
uint16_t getCurrentHour();

// Gets the current system minute
uint16_t getCurrentMinute();

#endif