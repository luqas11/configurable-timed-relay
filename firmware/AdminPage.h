#ifndef ADMIN_PAGE_H
#define ADMIN_PAGE_H

#include "Arduino.h"

// Page to manage and see the WiFi configuration
const String ADMIN_HTML = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Configuración WiFi</title>
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
      button:disabled {
        background: #77a;
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
      <h1>Configuración WiFi</h1>
      <p>
        Configuración de SSID y contraseña de la red WiFi a la que se conectará el dispositivo.
      </p>
      <form onsubmit="setValues(event)">
        <label>SSID:</label>
        <input type="text" id="ssid" required maxlength="32" /><br />
        <label>Contraseña:</label>
        <input type="password" id="password" required minlength="8" maxlength="64" /><br />
        <button type="submit" id="save">Guardar</button>
      </form>
      <h3>Configuración actual</h3>
      <p><b>SSID:</b> <span id="savedSSID">-</span></p>
      <p>
        <b>Estado de conexión:</b>
        <span id="connectionStatus">-</span>
      </p>
    </div>

    <script>
      function setValues(event) {
        event.preventDefault();
        document.getElementById("save").disabled = true;

        const ssid = document.getElementById("ssid").value;
        const password = document.getElementById("password").value;

        fetch("/set-wifi-config?ssid=" + ssid + "&password=" + password)
          .then((response) => {
            if (!response.ok) {
              throw new Error();
            } else {
              getValues();
            }
          })
          .catch((error) => {
            alert("No fue posible guardar la configuración indicada");
          })
          .finally(() => {
            document.getElementById("save").disabled = false;
          });
      }

      function getValues(setInputs) {
        fetch("/get-wifi-config")
          .then((response) => {
            if (!response.ok) {
              throw new Error("No fue posible obtener la configuración actual");
            } else {
              return response.json();
            }
          })
          .then((data) => {
            document.getElementById("savedSSID").innerText = data.ssid ?? "-";
            if (setInputs) {
              document.getElementById("ssid").value = data.ssid ?? "";
            }
          })
          .catch((error) => {
            console.error(error.message);
          });
      }

      function getStatusText(statusCode) {
        const statusMap = {
          0: "En espera",
          1: "Red no encontrada",
          3: "Conectado",
          4: "Conexión fallida",
          5: "Conexión perdida",
          6: "Contraseña incorrecta",
          7: "Desconectado"
        };
        return statusMap[statusCode] ?? "Desconocido";
      }

      function getStatus() {
        fetch("/get-wifi-status")
          .then((response) => {
            if (!response.ok) {
              throw new Error("No fue posible obtener el estado de la conexión");
            } else {
              return response.json();
            }
          })
          .then((data) => {
            const statusCode = data.status;
            const statusText = getStatusText(statusCode);
            document.getElementById("connectionStatus").innerText = statusText;
          })
          .catch((error) => {
            console.error(error.message);
          });
      }

      setInterval(getStatus, 1000);
      window.onload = () => {
        getValues(true);
        getStatus();
      };
    </script>
  </body>
</html>
)rawliteral";

#endif