#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "config.h"

ESP8266WebServer server(80);

int STATUS_LED = 2;

unsigned long lastLedTimestamp;
bool isLedBlinking;
const int LED_BLINK_TIME = 200;

const int TIME_ADDR = 0;
const int DURATION_ADDR = 1;

const int MAX_TIME = 60 * 24;
const int MAX_DURATION = 600;

void setup() {
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);

  EEPROM.begin(512);
  connectToWifi(SSID, PASSWORD, IP, GATEWAY, SUBNET, STATUS_LED);

  server.on("/", handleRoot);
  server.on("/set-values", handleSetValues);
  server.on("/get-values", handleGetValues);
  server.onNotFound(handleNotFound);

  server.begin();
}

void loop() {
  if (isLedBlinking != true) {
    digitalWrite(STATUS_LED, (WiFi.status() == WL_CONNECTED) ? LOW : HIGH);
  }

  if ((millis() - lastLedTimestamp) > LED_BLINK_TIME && isLedBlinking == true) {
    digitalWrite(STATUS_LED, LOW);
    isLedBlinking = false;
  }
  server.handleClient();
}

void connectToWifi(String ssid, String password, String ip, String gateway, String subnet, int ledPin) {
  IPAddress _ip;
  _ip.fromString(ip);
  IPAddress _gateway;
  _gateway.fromString(gateway);
  IPAddress _subnet;
  _subnet.fromString(subnet);

  Serial.print("Connecting to ");
  Serial.println(String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.config(_ip, _gateway, _subnet);
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
            <label>Hora:</label>
            <input type="time" id="time" required /><br />
            <label>Duración:</label>
            <input type="number" id="duration" min="1" max="600" required /><br />
            <button type="submit">Guardar</button>
          </form>
          <h3>Configuración actual</h3>
          <p><b>Hora:</b> <span id="savedTime">-</span></p>
          <p>
            <b>Duración:</b>
            <span id="savedDuration">-</span>
          </p>
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
