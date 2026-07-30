#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "my_wifi";
const char* password = "cool_turret1234";

WebServer server(80);

Servo servoPan;
Servo servoTilt;
Servo servoShoot;

const int servoPanPin = 21;
const int servoTiltPin = 18;
const int laserPin = 32;
const int motorsPin = 26; 
const int servoShootPin = 15;

int panPosition = 90;
int tiltPosition = 90;
int shootPosition = 90;
bool laserState = false;
bool motorsState = false;

const int minAngle = 0;
const int maxAngle = 180;

const char MAIN_page[] PROGMEM = R"====(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, orientation=landscape">
  <title>Contrôle de Tourelle</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 0;
      padding: 0;
      background-color: #1e1e1e;
      color: white;
    }
    h1 {
      margin-top: 20px;
    }
    #controller {
      position: relative;
      width: 90vmin;
      max-width: 300px;
      height: 90vmin;
      max-height: 300px;
      margin: 20px auto;
      background: #444;
      border-radius: 10px;
      touch-action: none;
    }
    #cross {
      position: absolute;
      width: 20px;
      height: 20px;
      background: red;
      border-radius: 50%;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      cursor: pointer;
    }
    .button {
      padding: 10px 20px;
      margin: 10px;
      font-size: 18px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
      margin: 10px;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: .4s;
      border-radius: 34px;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: .4s;
      border-radius: 50%;
    }
    input:checked + .slider {
      background-color: #2196F3;
    }
    input:checked + .slider:before {
      transform: translateX(26px);
    }
    #resetButton {
      background: #007bff;
      color: white;
    }
    #shootButton {
      background: #28a745;
      color: white;
    }
    #rafaleButton {
      background: #ffc107;
      color: white;
    }
    #info {
      font-size: 18px;
      margin-top: 10px;
    }
  </style>
</head>
<body>
  <h1>Contrôle de la Tourelle</h1>
  <div id="controller">
    <div id="cross"></div>
  </div>
  <p id="info">Pan: 90 | Tilt: 90</p>
  <button id="resetButton" class="button">Réinitialiser</button>
  <label>Motors:</label>
  <label class="switch">
    <input type="checkbox" id="motorsSwitch">
    <span class="slider"></span>
  </label>
  <label>Laser:</label>
  <label class="switch">
    <input type="checkbox" id="laserSwitch">
    <span class="slider"></span>
  </label>
  <button id="shootButton" class="button">Shoot</button>
  <button id="rafaleButton" class="button">Rafale</button>
  <script>
    const controller = document.getElementById('controller');
    const cross = document.getElementById('cross');
    const info = document.getElementById('info');
    const resetButton = document.getElementById('resetButton');
    const motorsSwitch = document.getElementById('motorsSwitch');
    const laserSwitch = document.getElementById('laserSwitch');
    const shootButton = document.getElementById('shootButton');
    const rafaleButton = document.getElementById('rafaleButton');
    let controllerRect = controller.getBoundingClientRect();
    let xPos = controllerRect.width / 2;
    let yPos = controllerRect.height / 2;
    let rafaleActive = false;

    function updateLayout() {
      controllerRect = controller.getBoundingClientRect();
    }
    
    window.addEventListener("orientationchange", updateLayout);
    window.addEventListener("resize", updateLayout);

    function sendCommand(pan, tilt) {
      fetch(`/move?pan=${pan}&tilt=${tilt}`)
        .catch(err => console.error("Erreur move: ", err));
      info.textContent = `Pan: ${pan} | Tilt: ${tilt}`;
    }

    function updatePosition(x, y) {
      xPos = Math.max(0, Math.min(controllerRect.width, x));
      yPos = Math.max(0, Math.min(controllerRect.height, y));
      cross.style.left = `${xPos}px`;
      cross.style.top = `${yPos}px`;
      let pan = Math.round(xPos / controllerRect.width * 180);
      let tilt = Math.round(yPos / controllerRect.height * 180);
      sendCommand(pan, tilt);
    }

    function handleMove(x, y) {
      let relativeX = x - controllerRect.left;
      let relativeY = y - controllerRect.top;
      updatePosition(relativeX, relativeY);
    }

    controller.addEventListener('touchstart', (e) => {
      e.preventDefault();
      handleMove(e.touches[0].clientX, e.touches[0].clientY);
    });

    controller.addEventListener('touchmove', (e) => {
      e.preventDefault();
      handleMove(e.touches[0].clientX, e.touches[0].clientY);
    });

    resetButton.addEventListener('click', () => {
      updatePosition(controllerRect.width / 2, controllerRect.height / 2);
    });

    motorsSwitch.addEventListener('change', () => {
      console.log("Motors switch changed to: " + motorsSwitch.checked); // Débogage
      fetch(`/motors?state=${motorsSwitch.checked ? 1 : 0}`)
        .catch(err => console.error("Erreur motors: ", err));
    });

    laserSwitch.addEventListener('change', () => {
      console.log("Laser switch changed to: " + laserSwitch.checked); // Débogage
      fetch(`/laser?state=${laserSwitch.checked ? 1 : 0}`)
        .catch(err => console.error("Erreur laser: ", err));
    });

    shootButton.addEventListener('click', () => {
      fetch('/shoot')
        .catch(err => console.error("Erreur shoot: ", err));
    });

    rafaleButton.addEventListener('mousedown', () => {
      rafaleActive = true;
      function rafaleLoop() {
        if (rafaleActive) {
          fetch('/shoot')
            .then(() => setTimeout(rafaleLoop, 200))
            .catch(err => console.error("Erreur rafale: ", err));
        }
      }
      rafaleLoop();
    });

    rafaleButton.addEventListener('mouseup', () => {
      rafaleActive = false;
    });

    rafaleButton.addEventListener('touchstart', (e) => {
      e.preventDefault();
      rafaleActive = true;
      function rafaleLoop() {
        if (rafaleActive) {
          fetch('/shoot')
            .then(() => setTimeout(rafaleLoop, 200))
            .catch(err => console.error("Erreur rafale: ", err));
        }
      }
      rafaleLoop();
    });

    rafaleButton.addEventListener('touchend', (e) => {
      e.preventDefault();
      rafaleActive = false;
    });
  </script>
</body>
</html>
)====";

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void handleMove() {
  if (server.hasArg("pan") && server.hasArg("tilt")) {
    panPosition = constrain(180 - server.arg("pan").toInt(), minAngle, maxAngle);
    int rawTilt = server.arg("tilt").toInt();
    tiltPosition = map(rawTilt, 0, 180, 30, 150);
    tiltPosition = constrain(tiltPosition, 30, 150);
    servoPan.write(panPosition);
    servoTilt.write(tiltPosition);
    Serial.println("Move - Pan: " + String(panPosition) + " | Tilt: " + String(tiltPosition));
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Paramètres manquants");
  }
}

void handleLaser() {
  if (server.hasArg("state")) {
    laserState = server.arg("state").toInt() == 1;
    digitalWrite(laserPin, laserState ? HIGH : LOW);
    Serial.println("Laser state: " + String(laserState ? "ON" : "OFF"));
    server.send(200, "text/plain", laserState ? "ON" : "OFF");
  } else {
    server.send(400, "text/plain", "Paramètre state manquant");
  }
}

void handleMotors() {
  if (server.hasArg("state")) {
    motorsState = server.arg("state").toInt() == 1;
    digitalWrite(motorsPin, motorsState ? HIGH : LOW);
    Serial.println("Motors state: " + String(motorsState ? "ON" : "OFF"));
    server.send(200, "text/plain", motorsState ? "ON" : "OFF");
  } else {
    server.send(400, "text/plain", "Paramètre state manquant");
  }
}

void handleShoot() {
  shootPosition = 0;
  servoShoot.write(shootPosition);
  delay(300);
  shootPosition = 180;
  servoShoot.write(shootPosition);
  delay(300);
  shootPosition = 90;
  servoShoot.write(shootPosition);
  Serial.println("Shoot effectué (aller-retour + recentrage)");
  server.send(200, "text/plain", "Tir effectué");
}



void setup() {
  Serial.begin(115200);

  pinMode(laserPin, OUTPUT);
  pinMode(motorsPin, OUTPUT);
  digitalWrite(laserPin, LOW);
  digitalWrite(motorsPin, LOW);

  servoPan.setPeriodHertz(50);
  servoTilt.setPeriodHertz(50);
  servoShoot.setPeriodHertz(50);
  servoPan.attach(servoPanPin, 500, 2400);
  servoTilt.attach(servoTiltPin, 500, 2400);
  servoShoot.attach(servoShootPin, 500, 2400);
  servoPan.write(panPosition);
  servoTilt.write(tiltPosition);
  servoShoot.write(shootPosition);

  WiFi.softAP("Tourelle_ESP32", "12345678");

  Serial.println("Point d'accès créé !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/laser", handleLaser);
  server.on("/motors", handleMotors);
  server.on("/shoot", handleShoot);

  server.begin();
  Serial.println("Serveur démarré");
}

void loop() {
  server.handleClient();
  digitalWrite(motorsPin, motorsState ? HIGH : LOW);
  digitalWrite(laserPin, laserState ? HIGH : LOW);
}
