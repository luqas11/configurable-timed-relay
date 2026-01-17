#include "WiFiUtils.h"
#include <ESP8266WiFi.h>

const int MAX_AWAIT_TIME = 10;

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
  Serial.print("Connecting to WiFi");
  for (int i = 0; i < MAX_AWAIT_TIME * 2; i++) {
    if (isWiFiConnected()) {
      break;
    }
    digitalWrite(ledPin, LOW);
    delay(250);
    Serial.print(".");
    digitalWrite(ledPin, HIGH);
    delay(250);
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