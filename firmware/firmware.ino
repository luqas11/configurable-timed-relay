#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "config.h"

static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = -3;

WiFiUDP Udp;
unsigned int localPort = 8888;

ESP8266WebServer server(80);

int STATUS_LED = 12;
int RELAY_PIN = 4;

unsigned long lastLedTimestamp;
bool isLedBlinking;
const int LED_BLINK_TIME = 200;

const int TIME_ADDR = 0;
const int DURATION_ADDR = 1;

const int MAX_TIME = 60 * 24;
const int MAX_DURATION = 600;

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

String currentHour = "-";
uint16_t currentTime = 0;
uint16_t currentDuration = 1;
unsigned long turnOffTimestamp;
bool hasStarted;
bool isOnTime;

time_t getNtpTime() {
  IPAddress ntpServerIP;

  while (Udp.parsePacket() > 0)
    ;
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
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

void setup() {
  Serial.begin(9600);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);
  digitalWrite(RELAY_PIN, LOW);

  EEPROM.begin(512);
  connectToWifi(SSID, PASSWORD, STATUS_LED);
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  currentTime = readFromEEPROM(TIME_ADDR);
  currentDuration = readFromEEPROM(DURATION_ADDR);

  server.on("/", handleRoot);
  server.on("/set-values", handleSetValues);
  server.on("/get-values", handleGetValues);
  server.on("/get-current-hour", handleGetCurrentHour);  
  server.onNotFound(handleNotFound);

  server.begin();
}

void loop() {
  if (timeStatus() != timeNotSet) {
    currentHour = hour() * 60 + minute();
  }

  if (millis() > turnOffTimestamp && hasStarted) {
    digitalWrite(RELAY_PIN, LOW);
  }

  if (isOnTime && !hasStarted) {
    turnOffTimestamp = millis() + currentDuration * 1000;
    digitalWrite(RELAY_PIN, HIGH);
    hasStarted = true;
  }

  if ((hour() == currentTime / 60) && (minute() == currentTime % 60)) {
    isOnTime = true;
  } else {
    isOnTime = false;
    hasStarted = false;
  }

  if (isLedBlinking != true) {
    digitalWrite(STATUS_LED, (WiFi.status() == WL_CONNECTED) ? LOW : HIGH);
  }

  if ((millis() - lastLedTimestamp) > LED_BLINK_TIME && isLedBlinking == true) {
    digitalWrite(STATUS_LED, LOW);
    isLedBlinking = false;
  }
  server.handleClient();
}

void sendNTPpacket(IPAddress &address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void connectToWifi(String ssid, String password, int ledPin) {
  Serial.print("Connecting to ");
  Serial.println(String(ssid));
  WiFi.mode(WIFI_STA);
  //WiFi.config(_ip, _gateway, _subnet);
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

void handleSetValues() {
  String time = server.arg("time");
  String duration = server.arg("duration");

  char *endTime;
  long newTime = strtol(time.c_str(), &endTime, 10);

  if (time == "" || *endTime != '\0' || newTime >= MAX_TIME || newTime <= 0) {
    String response;
    StaticJsonDocument<128> doc;

    doc["error_code"] = "ERR_01";
    doc["error_message"] = "Invalid time value";
    serializeJson(doc, response);

    blinkStatusLed();
    server.send(404, "application/json", response);
    return;
  }

  char *endDuration;
  long newDuration = strtol(duration.c_str(), &endDuration, 10);

  if (duration == "" || *endDuration != '\0' || newDuration >= MAX_DURATION || newDuration <= 0) {
    String response;
    StaticJsonDocument<128> doc;

    doc["error_code"] = "ERR_02";
    doc["error_message"] = "Invalid duration value";
    serializeJson(doc, response);

    blinkStatusLed();
    server.send(404, "application/json", response);
    return;
  }

  writeToEEPROM(TIME_ADDR, newTime);
  writeToEEPROM(DURATION_ADDR, newDuration);
  currentTime = newTime;
  currentDuration = newDuration;

  server.send(200);
  blinkStatusLed();
}

void handleGetValues() {
  String response;
  StaticJsonDocument<128> doc;

  uint16_t time = readFromEEPROM(TIME_ADDR);
  uint16_t duration = readFromEEPROM(DURATION_ADDR);

  doc["time"] = time;
  doc["duration"] = duration;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  blinkStatusLed();
}

void handleGetCurrentHour() {
  String response;
  StaticJsonDocument<128> doc;

  doc["currentHour"] = currentHour;
  serializeJson(doc, response);

  server.send(200, "application/json", response);
  blinkStatusLed();
}

void handleNotFound() {
  String response;
  StaticJsonDocument<128> doc;

  doc["error_code"] = "ERR_00";
  doc["error_message"] = "URL not found";
  serializeJson(doc, response);

  server.send(404, "application/json", response);
  blinkStatusLed();
}

void handleRoot() {
  String savedTime = "00:00";
  int savedNumber = 0;

  String html = R"rawliteral(
    <!DOCTYPE html>
      <html>
        <head>
          <meta charset="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1.0" />
          <title>Riego automático</title>
          <style>
            body {
              font-family: Arial, sans-serif;
              text-align: center;
              background: #111;
              padding: 20px;
            }
            .container {
              background: #333;
              padding: 20px;
              border-radius: 8px;
              max-width: 400px;
              margin: auto;
            }
            input,
            button {
              padding: 10px;
              margin: 10px;
              font-size: 16px;
              width: 80%;
            }
            button {
              background: #116;
              border: none;
              border-radius: 5px;
              cursor: pointer;
            }
            button:hover {
              background: #118;
            }
            input {
              background: lightgray;
            }
            p,
            span,
            button,
            h1,
            h3,
            title,
            label {
              color: lightgray;
            }
          </style>
        </head>
        <body>
          <div class="container">
            <h1>Riego automático</h1>
            <p>
              Configuración de horario y duración (en segundos) del riego automático.
              La duración máxima permitida es de 600 segundos.
            </p>
            <form onsubmit="setValues(event)">
              <label>Hora de riego:</label>
              <input type="time" id="time" required /><br />
              <label>Duración:</label>
              <input type="number" id="duration" min="1" max="600" required /><br />
              <button type="submit">Guardar</button>
            </form>
            <h3>Configuración actual</h3>
            <p><b>Hora de riego:</b> <span id="savedTime">-</span></p>
            <p>
              <b>Duración:</b>
              <span id="savedDuration">-</span>
            </p>
            <p><b>Hora del sistema:</b> <span id="currentHour">-</span></p>
          </div>

          <script>
            function setValues(event) {
              event.preventDefault();

              const duration = document.getElementById("duration").value;
              let time = document.getElementById("time").value;
              const [hours, minutes] = time.split(":").map(Number);
              time = hours * 60 + minutes;

              fetch("/set-values?time=" + time + "&duration=" + duration)
                .then((response) => {
                  if (!response.ok) {
                    const errorCode = response.json().code;
                    let errorMessage = "No fue posible guardar los valores indicados";
                    if (errorCode == "ERR_01") {
                      errorMessage = "La hora indicada es inválido";
                    }
                    if (errorCode == "ERR_02") {
                      errorMessage = "La duración indicada es inválida";
                    }
                    throw new Error(errorMessage);
                  } else {
                    getValues();
                  }
                })
                .catch((error) => {
                  alert(error.message);
                });
            }

            function getValues(setInputs) {
              fetch("/get-values")
                .then((response) => {
                  if (!response.ok) {
                    throw new Error("No fue posible obtener los valores actuales");
                  } else {
                    return response.json();
                  }
                })
                .then((data) => {
                  const hours = Math.floor(data.time / 60);
                  const minutes = data.time % 60;
                  const time =
                    String(hours).padStart(2, "0") +
                    ":" +
                    String(minutes).padStart(2, "0");

                  document.getElementById("savedTime").innerText = time ?? "-";
                  document.getElementById("savedDuration").innerText =
                    data.duration ?? "-";
                  if (setInputs) {
                    document.getElementById("time").value = time ?? "00:00";
                    document.getElementById("duration").value = data.duration ?? 1;
                  }
                })
                .catch((error) => {
                  alert(error.message);
                });
            }

            function getCurrentHour() {
              fetch("/get-current-hour")
                .then((response) => {
                  if (!response.ok) {
                    throw new Error("No fue posible obtener la hora del sistema");
                  } else {
                    return response.json();
                  }
                })
                .then((data) => {
                  const hours = Math.floor(data.currentHour / 60);
                  const minutes = data.currentHour % 60;
                  const time =
                    String(hours).padStart(2, "0") +
                    ":" +
                    String(minutes).padStart(2, "0");

                  document.getElementById("currentHour").innerText = time ?? "00:00";
                })
                .catch((error) => {
                  console.log(error.message);
                });
            }

            setInterval(getCurrentHour, 1000);
            window.onload = () => getValues(true);
          </script>
        </body>
      </html>
    )rawliteral";

  server.send(200, "text/html", html);
}

uint16_t readFromEEPROM(int index) {
  int _index = index * sizeof(uint16_t);
  uint16_t value;
  EEPROM.get(_index, value);
  return value;
}

void writeToEEPROM(int index, uint16_t value) {
  int _index = index * sizeof(uint16_t);
  EEPROM.put(_index, value);
  EEPROM.commit();
}

void blinkStatusLed() {
  digitalWrite(STATUS_LED, HIGH);
  lastLedTimestamp = millis();
  isLedBlinking = true;
}
