#include "WiFiUtils.h"
#include <ESP8266WiFi.h>

const int MAX_AWAIT_TIME = 15;

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void beginWiFi(String ssidAP, String passwordAP, int ledPin) {
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin();
  WiFi.softAP(ssidAP, passwordAP);
  WiFi.persistent(true);
  connectToWiFi(ledPin);
}

void connectToWiFi(int ledPin) {
  Serial.println("Connecting to WiFi");
  for (int i = 0; i < MAX_AWAIT_TIME; i++) {
    if (isWiFiConnected()) {
      break;
    }
    digitalWrite(ledPin, LOW);
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, HIGH);
    delay(500);
  }
  Serial.println();

  if (isWiFiConnected()) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed");
  }
}