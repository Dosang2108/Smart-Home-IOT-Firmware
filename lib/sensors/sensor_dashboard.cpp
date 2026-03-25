#include "sensor_dashboard.hpp"
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "global_var.hpp"
#include "control.hpp"
#include "MQTT.hpp"

static WebServer dashboardServer(80);
static bool dashboardStarted = false;

// =======================================================================
// LƯU TRỮ HTML/CSS/JS VÀO FLASH (PROGMEM) ĐỂ TIẾT KIỆM RAM
// =======================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>YoloHome Dashboard</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: #eef2f5; color: #333; }
    h2 { text-align: center; color: #2c3e50; }
    .container { max-width: 600px; margin: 0 auto; }
    .card { background: #fff; padding: 20px; border-radius: 12px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); margin-bottom: 20px; }
    h3 { margin-top: 0; color: #34495e; border-bottom: 2px solid #eee; padding-bottom: 10px; }
    
    /* Sensor Grid */
    .sensor-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; }
    .sensor-box { background: #f8f9fa; padding: 15px; border-radius: 8px; text-align: center; }
    .sensor-val { font-size: 24px; font-weight: bold; color: #007bff; margin: 5px 0; }
    .sensor-label { font-size: 14px; color: #6c757d; }

    /* Controls */
    .control-group { margin-bottom: 15px; display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; }
    button { padding: 10px 15px; border: none; border-radius: 6px; background: #007bff; color: white; font-weight: bold; cursor: pointer; transition: 0.3s; }
    button:hover { background: #0056b3; }
    button.btn-danger { background: #dc3545; }
    button.btn-danger:hover { background: #c82333; }
    button.btn-success { background: #28a745; }
    
    /* Slider & Color Picker */
    input[type="range"] { flex: 1; margin: 0 15px; cursor: pointer; }
    input[type="color"] { width: 50px; height: 40px; border: none; border-radius: 5px; cursor: pointer; background: transparent; }
  </style>
</head>
<body>
  <div class="container">
    <h2>🏠 YoloHome Local</h2>
    
    <div class="card">
      <h3>📊 Thông số Môi trường</h3>
      <div class="sensor-grid">
        <div class="sensor-box">
          <div class="sensor-label">Nhiệt độ</div>
          <div class="sensor-val"><span id="t">--</span>&deg;C</div>
        </div>
        <div class="sensor-box">
          <div class="sensor-label">Độ ẩm</div>
          <div class="sensor-val"><span id="h">--</span>%</div>
        </div>
        <div class="sensor-box">
          <div class="sensor-label">Ánh sáng</div>
          <div class="sensor-val"><span id="l">--</span></div>
        </div>
        <div class="sensor-box">
          <div class="sensor-label">Chuyển động</div>
          <div class="sensor-val" id="p" style="font-size:18px;">--</div>
        </div>
      </div>
    </div>

    <div class="card">
      <h3>🎛️ Bảng Điều khiển</h3>
      
      <div class="control-group">
        <label><b>Đèn RGB:</b></label>
        <input type="color" id="rgbColor" value="#ff0000" onchange="pickColor(this.value)">
        <button onclick="rgbAuto()">Tự động</button>
        <button class="btn-danger" onclick="rgbOff()">Tắt Đèn</button>
      </div>

      <div class="control-group">
        <label><b>Quạt:</b> <span id="fanVal">0</span>%</label>
        <input type="range" id="fanSlider" min="0" max="100" value="0" onchange="setFan(this.value)">
      </div>

      <div class="control-group" style="justify-content: center; margin-top: 20px;">
        <button class="btn-success" onclick="openDoor()" style="width: 100%; font-size: 16px;">🔓 MỞ CỬA CHÍNH</button>
      </div>
    </div>
  </div>

  <script>
    function refreshSensors() {
      fetch('/api/sensors').then(r => r.json()).then(d => {
        document.getElementById('t').textContent = d.temperature.toFixed(1);
        document.getElementById('h').textContent = d.humidity.toFixed(1);
        document.getElementById('l').textContent = d.light;
        
        let p = document.getElementById('p');
        if(d.pirDetected) { p.textContent = 'Phát hiện!'; p.style.color = 'red'; } 
        else { p.textContent = 'An toàn'; p.style.color = '#28a745'; }
        
        // Không cập nhật slider nếu user đang kéo, chỉ cập nhật text
        document.getElementById('fanVal').textContent = d.fanSpeed;
      }).catch(e => console.log("Lỗi tải dữ liệu", e));
    }

    function pickColor(hex) {
      // Chuyển hex sang rgb
      let r = parseInt(hex.slice(1, 3), 16);
      let g = parseInt(hex.slice(3, 5), 16);
      let b = parseInt(hex.slice(5, 7), 16);
      fetch(`/api/rgb?r=${r}&g=${g}&b=${b}`);
    }
    
    function rgbOff() { fetch('/api/rgb/off'); }
    function rgbAuto() { fetch('/api/rgb/auto'); }
    function setFan(v) { fetch(`/api/fan?speed=${v}`); document.getElementById('fanVal').textContent = v; }
    function openDoor() { fetch('/api/door'); alert('Đang mở cửa!'); }

    setInterval(refreshSensors, 2000);
    refreshSensors();
  </script>
</body>
</html>
)rawliteral";

// =======================================================================
// CÁC HÀM XỬ LÝ API
// =======================================================================

static void handleApiSensors()
{
  JsonDocument doc;
  doc["temperature"] = Value_Temperature;
  doc["humidity"] = Value_Humidity;
  doc["light"] = Value_Light;
  doc["pirDetected"] = pirDetected;
  doc["dhtValid"] = dhtDataValid;
  doc["mqttConnected"] = isMqttConnected();
  doc["ledEnabled"] = mqttLedState;
  doc["fanSpeed"] = mqttFanSpeed;

  String payload;
  serializeJson(doc, payload);
  
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
    mqttFanSpeed = constrain(dashboardServer.arg("speed").toInt(), 0, 100);
    fan_set_speed(mqttFanSpeed);
  }
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Fan set");
}

static void handleDoorOpen()
{
  servo_open();
  doorState = DOOR_OPENING; // Biến từ global_var
  doorOpenTime = millis_present;
  
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Door Opened");
}

static void handleDashboardRoot()
{
  // Gửi nội dung trang web từ bộ nhớ Flash (PROGMEM)
  dashboardServer.send_P(200, "text/html", index_html);
}


void initSensorDashboard()
{
  if (dashboardStarted) return;
  
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
  dashboardServer.on("/api/door", HTTP_GET, handleDoorOpen); // Thêm API Mở cửa

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