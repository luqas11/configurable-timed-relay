#include "WiFiUtils.h"
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = -3;

WiFiUDP Udp;
unsigned int localPort = 8888;

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

bool isWiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void connectToWifi(String ssid, String password, int ledPin) {
  Serial.print("Connecting to ");
  Serial.println(String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, LOW);
    delay(250);
    Serial.print(".");
    digitalWrite(ledPin, HIGH);
    delay(250);
  }
  digitalWrite(ledPin, LOW);

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

time_t getNtpTime() {
  IPAddress ntpServerIP;

  while (Udp.parsePacket() > 0);
  WiFi.hostByName(ntpServerName, ntpServerIP);
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(ntpServerIP, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  return 0;
}

void beginTime() {
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

bool isTimeSet(){
  return timeStatus() != timeNotSet;
}

uint16_t getCurrentHour(){
  return hour();
}

uint16_t getCurrentMinute(){
  return minute();
}
