#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "EEPROMUtils.h"
#include "WiFiUtils.h"
#include "TimeUtils.h"
#include "config.h"
#include "AdminPage.h"
#include "IndexPage.h"

/*
   Version 1.1.0
   16-1-2026

   External libraries:
   ArduinoJson@7.4.2
   esp8266@3.1.2
   Time@1.6.1
*/

// Duration of the status LED blinks
const int LED_BLINK_TIME = 200;

// Outputs pin numbers
int STATUS_LED = 12;
int RELAY_PIN = 4;

// Memory addresses
const int TIME_ADDR = 0;
const int DURATION_ADDR = 1;

// Maximum configurable values
const int MAX_TIME = 60 * 24;
const int MAX_DURATION = 600;

// State variables
unsigned long lastLedTimestamp;
bool isLedBlinking;
int currentTime = -1;
int currentDuration = -1;
unsigned long turnOffTimestamp;
bool hasAlreadyRun;

// HTTP server instance
ESP8266WebServer server(80);

void setup() {
  // Initialize output pins
  pinMode(STATUS_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  digitalWrite(RELAY_PIN, LOW);

  // Initialize the serial communication, the EEPROM library, the WiFi connection and the Time library
  Serial.begin(9600);
  beginEEPROM();
  beginWiFi(SSID_AP, PASSWORD_AP, STATUS_LED);
  beginTime();

  // Read and validate the configured values from the EEPROM
  uint16_t _currentTime = readFromEEPROM(TIME_ADDR);
  uint16_t _currentDuration = readFromEEPROM(DURATION_ADDR);
  if (_currentTime < MAX_TIME) {
    currentTime = _currentTime;
  }
  if (_currentDuration < MAX_DURATION) {
    currentDuration = _currentDuration;
  }

  // Set up all the server endpoints
  server.on("/", handleRoot);
  server.on("/admin", handleAdmin);
  server.on("/set-values", handleSetValues);
  server.on("/get-values", handleGetValues);
  server.on("/get-current-hour", handleGetCurrentHour);
  server.on("/get-wifi-config", handleGetWiFiConfig);
  server.on("/set-wifi-config", handleSetWiFiConfig);
  server.on("/get-wifi-status", handleGetWiFiStatus);
  server.onNotFound(handleNotFound);

  // Initialize the HTTP server
  server.begin();
}

void loop() {
  // Handle any incoming request, if exists
  server.handleClient();

  // Set the LED state depending on the network status
  if (isLedBlinking != true) {
    digitalWrite(STATUS_LED, isWiFiConnected() ? LOW : HIGH);
  }

  // Turn off the status LED if the blink is finished
  if ((millis() - lastLedTimestamp) > LED_BLINK_TIME && isLedBlinking == true) {
    digitalWrite(STATUS_LED, LOW);
    isLedBlinking = false;
  }

  // Check if the current minute is the confgured one
  if ((getCurrentHour() == currentTime / 60) && (getCurrentMinute() == currentTime % 60)) {
    // If the output hasn't been turned on yet, do it and update the state variables
    if (!hasAlreadyRun) {
      turnOffTimestamp = millis() + currentDuration * 1000;
      digitalWrite(RELAY_PIN, HIGH);
      hasAlreadyRun = true;
    }
  } else {
    // When the configured minute is over, and the output is off, update the state variable
    if (millis() > turnOffTimestamp) {
      hasAlreadyRun = false;
    }
  }

  // If the output timer is over, turn the output off
  if (millis() > turnOffTimestamp) {
    digitalWrite(RELAY_PIN, LOW);
  }
}

// Sets the configured values
void handleSetValues() {
  String time = server.arg("time");
  String duration = server.arg("duration");

  char *endTime;
  long newTime = strtol(time.c_str(), &endTime, 10);

  if (time == "" || *endTime != '\0' || newTime >= MAX_TIME || newTime <= 0) {
    String response = formatError("ERR_01", "Invalid time value");
    server.send(400, "application/json", response);
    blinkStatusLed();
    return;
  }

  char *endDuration;
  long newDuration = strtol(duration.c_str(), &endDuration, 10);

  if (duration == "" || *endDuration != '\0' || newDuration >= MAX_DURATION || newDuration <= 0) {
    String response = formatError("ERR_02", "Invalid duration value");
    server.send(400, "application/json", response);
    blinkStatusLed();
    return;
  }

  writeToEEPROM(TIME_ADDR, newTime);
  writeToEEPROM(DURATION_ADDR, newDuration);
  currentTime = newTime;
  currentDuration = newDuration;

  server.send(200);
  blinkStatusLed();
}

// Gets the configured values
void handleGetValues() {
  if (currentTime == -1 || currentDuration == -1) {
    String response = formatError("ERR_04", "Cannot get the configured values");
    server.send(500, "application/json", response);
    blinkStatusLed();
    return;
  }

  String response;
  StaticJsonDocument<128> doc;

  doc["time"] = currentTime;
  doc["duration"] = currentDuration;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  blinkStatusLed();
}

// Gets the current system hour
void handleGetCurrentHour() {
  if (!isTimeSet()) {
    String response = formatError("ERR_03", "The system hour is not set");
    server.send(500, "application/json", response);
    blinkStatusLed();
    return;
  }

  String response;
  StaticJsonDocument<128> doc;

  doc["currentHour"] = getCurrentHour() * 60 + getCurrentMinute();
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  blinkStatusLed();
}

// Sets the WiFi configuration
void handleSetWiFiConfig() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  if (ssid == "" || ssid.length() > 32) {
    String response = formatError("ERR_05", "Invalid WiFi SSID");
    server.send(400, "application/json", response);
    blinkStatusLed();
    return;
  }

  if (password == "" || password.length() < 8 || password.length() > 64) {
    String response = formatError("ERR_06", "Invalid WiFi password");
    server.send(400, "application/json", response);
    blinkStatusLed();
    return;
  }

  WiFi.begin(ssid, password);

  server.send(200);
  blinkStatusLed();
}

// Gets the WiFi configuration
void handleGetWiFiConfig() {
  String response;
  StaticJsonDocument<128> doc;

  doc["ssid"] = WiFi.SSID();
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  blinkStatusLed();
}

// Gets the current WiFi status
void handleGetWiFiStatus() {
  String response;
  StaticJsonDocument<128> doc;

  doc["status"] = WiFi.status();
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  blinkStatusLed();
}

// Returns a not found message to the client, formatted as a JSON string
void handleNotFound() {
  String response = formatError("ERR_00", "URL not found");
  server.send(404, "application/json", response);
  blinkStatusLed();
}

// Returns an HTML page to the client, to manage and see the system configuration
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
  blinkStatusLed();
}

// Returns an HTML page to the client, to manage and see the WiFi configuration
void handleAdmin() {
  server.send(200, "text/html", ADMIN_HTML);
  blinkStatusLed();
}

// Set the status LED to blink, updating it's state variables
void blinkStatusLed() {
  digitalWrite(STATUS_LED, HIGH);
  lastLedTimestamp = millis();
  isLedBlinking = true;
}

// Creates a JSON string with an error code and an error message values.
String formatError(String errorCode, String errorMessage) {
  String response;
  StaticJsonDocument<128> doc;

  doc["error_code"] = errorCode;
  doc["error_message"] = errorMessage;

  serializeJson(doc, response);
  return response;
}
