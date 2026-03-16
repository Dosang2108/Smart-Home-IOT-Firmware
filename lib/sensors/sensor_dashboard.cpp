#include "sensor_dashboard.hpp"

#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "global_var.hpp"
#include "control.hpp"
#include "MQTT.hpp"

static WebServer dashboardServer(80);
static bool dashboardStarted = false;

static void handleApiSensors()
{
  JsonDocument doc;
  doc["temperature"] = Value_Temperature;
  doc["humidity"] = Value_Humidity;
  doc["light"] = Value_Light;
  // doc["soilMoisture"] = Value_SoilMoisture;
  doc["pirDetected"] = pirDetected;
  doc["dhtValid"] = dhtDataValid;
  doc["mqttConnected"] = isMqttConnected();
  doc["ledEnabled"] = mqttLedState;
  doc["fanSpeed"] = mqttFanSpeed;

  String payload;
  serializeJson(doc, payload);
  
  // Cho phép Web bên ngoài (Front-end) truy cập API
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "application/json", payload);
}

static void handleRgbSet()
{
  int r = dashboardServer.hasArg("r") ? dashboardServer.arg("r").toInt() : 255;
  int g = dashboardServer.hasArg("g") ? dashboardServer.arg("g").toInt() : 255;
  int b = dashboardServer.hasArg("b") ? dashboardServer.arg("b").toInt() : 255;

  mqttLedState = true;
  mqttLedR = r; mqttLedG = g; mqttLedB = b;
  led_rgb_set_auto(false);
  led_rgb_set(r, g, b);
  
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "RGB set");
}

static void handleRgbOff()
{
  mqttLedState = false;
  led_rgb_set_auto(false);
  led_rgb_off();
  
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "RGB off");
}

static void handleRgbAuto()
{
  mqttLedState = true;
  led_rgb_set_auto(true);
  
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "RGB auto");
}

static void handleFanSet()
{
  if (dashboardServer.hasArg("speed")) {
    mqttFanSpeed = dashboardServer.arg("speed").toInt();
    fan_set_speed(mqttFanSpeed);
  }
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Fan set");
}


static void handleDashboardRoot()
{
  String html = F("<html><head><title>YoloHome Dashboard</title>"
  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
  "<style>body{font-family:sans-serif;margin:0;padding:20px;background:#f4f4f4;}"
  ".card{background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 5px rgba(0,0,0,0.1);margin-bottom:20px;}"
  "button{padding:10px 15px;border:none;border-radius:5px;background:#007bff;color:#fff;cursor:pointer;margin:5px;}"
  "button:hover{background:#0056b3;}</style></head><body>"
  "<h2>YoloHome Local Dashboard</h2>"
  "<div class='card'><h3>Sensors</h3>"
  "<p>Temp: <span id='t'>--</span> &deg;C</p>"
  "<p>Hum: <span id='h'>--</span> %</p>"
  "<p>Light: <span id='l'>--</span> %</p>"
  "<p>Motion: <span id='p'>--</span></p></div>"
  "<div class='card'><h3>Controls</h3>"
  "<button onclick='setRgb(255,0,0)'>Red</button>"
  "<button onclick='setRgb(0,255,0)'>Green</button>"
  "<button onclick='setRgb(0,0,255)'>Blue</button>"
  "<button onclick='rgbAuto()'>Auto RGB</button>"
  "<button onclick='rgbOff()'>LED Off</button><br><br>"
  "Fan: <button onclick='setFan(0)'>Off</button>"
  "<button onclick='setFan(50)'>50%</button>"
  "<button onclick='setFan(100)'>100%</button>"
  " (Current: <span id='fanVal'>0</span>%)<br><br></div>"
  "<script>"
  "function refreshSensors(){"
  "fetch('/api/sensors').then(r=>r.json()).then(d=>{"
  "document.getElementById('t').textContent=d.temperature;"
  "document.getElementById('h').textContent=d.humidity;"
  "document.getElementById('l').textContent=d.light;"
  "document.getElementById('p').textContent=d.pirDetected?'Yes':'No';"
  "document.getElementById('fanVal').textContent=d.fanSpeed;"
  "});}"
  "function setRgb(r,g,b){fetch('/api/rgb?r='+r+'&g='+g+'&b='+b);}"
  "function rgbOff(){fetch('/api/rgb/off');}"
  "function rgbAuto(){fetch('/api/rgb/auto');}"
  "function setFan(v){fetch('/api/fan?speed='+v);}"
  "setInterval(refreshSensors,2000);refreshSensors();"
  "</script></body></html>");

  dashboardServer.send(200, "text/html", html);
}

void initSensorDashboard()
{
  if (dashboardStarted) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sensor dashboard skipped: WiFi not connected");
    return;
  }

  dashboardServer.on("/", HTTP_GET, handleDashboardRoot);
  dashboardServer.on("/api/sensors", HTTP_GET, handleApiSensors);
  dashboardServer.on("/api/rgb", HTTP_GET, handleRgbSet);
  dashboardServer.on("/api/rgb/off", HTTP_GET, handleRgbOff);
  dashboardServer.on("/api/rgb/auto", HTTP_GET, handleRgbAuto);
  dashboardServer.on("/api/fan", HTTP_GET, handleFanSet);

  dashboardServer.begin();
  dashboardStarted = true;
  Serial.println("Sensor dashboard started on port 80");
}

void handleSensorDashboard()
{
  if (dashboardStarted) {
    dashboardServer.handleClient();
  }
}