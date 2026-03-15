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
  doc["soilMoisture"] = Value_SoilMoisture;
  doc["pirDetected"] = pirDetected;
  doc["dhtValid"] = dhtDataValid;
  doc["mqttConnected"] = isMqttConnected();
  doc["ledEnabled"] = mqttLedState;
  doc["fanSpeed"] = mqttFanSpeed;

  String payload;
  serializeJson(doc, payload);
  dashboardServer.send(200, "application/json", payload);
}

static void handleRgbSet()
{
  int r = dashboardServer.hasArg("r") ? dashboardServer.arg("r").toInt() : 255;
  int g = dashboardServer.hasArg("g") ? dashboardServer.arg("g").toInt() : 255;
  int b = dashboardServer.hasArg("b") ? dashboardServer.arg("b").toInt() : 255;

  mqttLedR = (uint8_t)constrain(r, 0, 255);
  mqttLedG = (uint8_t)constrain(g, 0, 255);
  mqttLedB = (uint8_t)constrain(b, 0, 255);

  mqttLedState = true;
  led_rgb_set_auto(false);
  led_rgb_set(mqttLedR, mqttLedG, mqttLedB);

  dashboardServer.send(200, "text/plain", "RGB updated");
}

static void handleRgbOff()
{
  mqttLedState = false;
  led_rgb_set_auto(false);
  led_rgb_off();
  ledwhite_off();
  dashboardServer.send(200, "text/plain", "RGB off");
}

static void handleRgbAuto()
{
  mqttLedState = true;
  led_rgb_set_auto(true);
  dashboardServer.send(200, "text/plain", "RGB auto enabled");
}

static void handleFanSet()
{
  int speed = dashboardServer.hasArg("speed") ? dashboardServer.arg("speed").toInt() : 0;
  mqttFanSpeed = constrain(speed, 0, 100);
  fan_set_speed(mqttFanSpeed);
  dashboardServer.send(200, "text/plain", "Fan updated");
}

static void handlePumpSet()
{
  bool on = dashboardServer.hasArg("on") && dashboardServer.arg("on") == "1";
  if (on) {
    pump_on();
  } else {
    pump_off();
  }
  dashboardServer.send(200, "text/plain", "Pump updated");
}

static void handleDashboardRoot()
{
  String html;
  html.reserve(5000);

  html += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>YoloHome Sensor Dashboard</title>";
  html += "<style>";
  html += "body{font-family:Segoe UI,Arial,sans-serif;background:#f6f7fb;margin:0;padding:16px;}";
  html += ".wrap{max-width:860px;margin:0 auto;}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:12px;}";
  html += ".card{background:#fff;padding:12px;border-radius:12px;box-shadow:0 2px 10px rgba(0,0,0,.07);}";
  html += "h1{margin:0 0 12px 0;font-size:22px;}";
  html += "h2{font-size:16px;margin:16px 0 8px 0;}";
  html += "button{padding:8px 10px;margin:4px;border:0;border-radius:8px;background:#1f6feb;color:#fff;cursor:pointer;}";
  html += "button.gray{background:#555;}";
  html += "input[type=number],input[type=range]{width:100%;}";
  html += "small{color:#666;}";
  html += "</style></head><body><div class='wrap'>";

  html += "<h1>YoloHome Sensor Dashboard</h1>";
  html += "<small>Auto refresh 2s | IP: ";
  html += WiFi.localIP().toString();
  html += "</small>";

  html += "<div class='grid'>";
  html += "<div class='card'><b>Temperature</b><div id='temperature'>--</div></div>";
  html += "<div class='card'><b>Humidity</b><div id='humidity'>--</div></div>";
  html += "<div class='card'><b>Light</b><div id='light'>--</div></div>";
  html += "<div class='card'><b>Soil</b><div id='soil'>--</div></div>";
  html += "<div class='card'><b>PIR</b><div id='pir'>--</div></div>";
  html += "<div class='card'><b>MQTT</b><div id='mqtt'>--</div></div>";
  html += "</div>";

  html += "<h2>RGB</h2>";
  html += "<button onclick=\"setRgb(255,0,0)\">Red</button>";
  html += "<button onclick=\"setRgb(0,255,0)\">Green</button>";
  html += "<button onclick=\"setRgb(0,0,255)\">Blue</button>";
  html += "<button onclick=\"setRgb(255,255,255)\">White</button>";
  html += "<button onclick=\"rgbAuto()\">Auto</button>";
  html += "<button class='gray' onclick=\"rgbOff()\">Off</button>";

  html += "<h2>Fan</h2>";
  html += "<input id='fan' type='range' min='0' max='100' value='0' oninput='fanVal.textContent=this.value' onchange='setFan(this.value)'>";
  html += "<div>Speed: <span id='fanVal'>0</span>%</div>";

  html += "<h2>Pump</h2>";
  html += "<button onclick=\"setPump(1)\">Pump ON</button>";
  html += "<button class='gray' onclick=\"setPump(0)\">Pump OFF</button>";

  html += "<script>";
  html += "async function refreshSensors(){";
  html += "const res=await fetch('/api/sensors');const d=await res.json();";
  html += "document.getElementById('temperature').textContent=d.dhtValid?d.temperature.toFixed(1)+' C':'--';";
  html += "document.getElementById('humidity').textContent=d.dhtValid?d.humidity.toFixed(1)+' %':'--';";
  html += "document.getElementById('light').textContent=d.light+' %';";
  html += "document.getElementById('soil').textContent=d.soilMoisture+' %';";
  html += "document.getElementById('pir').textContent=d.pirDetected?'Motion':'No motion';";
  html += "document.getElementById('mqtt').textContent=d.mqttConnected?'Connected':'Disconnected';";
  html += "document.getElementById('fan').value=d.fanSpeed;document.getElementById('fanVal').textContent=d.fanSpeed;";
  html += "}";
  html += "function setRgb(r,g,b){fetch('/api/rgb?r='+r+'&g='+g+'&b='+b);} ";
  html += "function rgbOff(){fetch('/api/rgb/off');} ";
  html += "function rgbAuto(){fetch('/api/rgb/auto');} ";
  html += "function setFan(v){fetch('/api/fan?speed='+v);} ";
  html += "function setPump(v){fetch('/api/pump?on='+v);} ";
  html += "setInterval(refreshSensors,2000);refreshSensors();";
  html += "</script>";

  html += "</div></body></html>";

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
  dashboardServer.on("/api/pump", HTTP_GET, handlePumpSet);

  dashboardServer.begin();
  dashboardStarted = true;

  Serial.print("Sensor dashboard: http://");
  Serial.println(WiFi.localIP());
}

void handleSensorDashboard()
{
  if (!dashboardStarted || WiFi.status() != WL_CONNECTED) {
    return;
  }
  dashboardServer.handleClient();
}
