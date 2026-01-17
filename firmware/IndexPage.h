#ifndef INDEX_PAGE_H
#define INDEX_PAGE_H

#include "Arduino.h"

// Page to manage and see the system configuration
const String INDEX_HTML = R"rawliteral(
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
        <button type="submit" id="save">Guardar</button>
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
        document.getElementById("save").disabled = true;

        const duration = document.getElementById("duration").value;
        let time = document.getElementById("time").value;
        const [hours, minutes] = time.split(":").map(Number);
        time = hours * 60 + minutes;

        fetch("/set-values?time=" + time + "&duration=" + duration)
          .then((response) => {
            if (!response.ok) {
              throw new Error();
            } else {
              document.getElementById("savedTime").innerText = "-";
              document.getElementById("savedDuration").innerText = "-";
              getValues();
            }
          })
          .catch((error) => {
            alert("No fue posible guardar los valores indicados");
          })
          .finally(() => {
            document.getElementById("save").disabled = false;
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
            console.error(error.message);
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
            console.error(error.message);
          });
      }

      setInterval(getCurrentHour, 1000);
      window.onload = () => {
        getValues(true);
        getCurrentHour();
      };
    </script>
  </body>
</html>
)rawliteral";

#endif