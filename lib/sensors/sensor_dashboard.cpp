#include "sensor_dashboard.hpp"
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "global_var.hpp"
#include "control.hpp"
#include "MQTT.hpp"

static WebServer dashboardServer(80);
static bool dashboardStarted = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>YoloHome Dashboard</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 10px; background: #eef2f5; color: #333; }
    h2 { text-align: center; color: #2c3e50; }
    .container { max-width: 800px; margin: 0 auto; }
    .card { background: #fff; padding: 20px; border-radius: 12px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); margin-bottom: 20px; }
    h3 { margin-top: 0; color: #34495e; border-bottom: 2px solid #eee; padding-bottom: 10px; }
    
    /* Layout Biểu đồ */
    .chart-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }
    .chart-box { background: #f8f9fa; padding: 10px; border-radius: 8px; text-align: center; }
    .sensor-val { font-size: 20px; font-weight: bold; margin: 5px 0; }
    canvas { max-width: 100%; height: 120px !important; }

    /* Controls */
    .control-group { margin-bottom: 15px; display: flex; align-items: center; justify-content: space-between; flex-wrap: wrap; background: #f8f9fa; padding: 15px; border-radius: 8px;}
    .control-actions { display: flex; gap: 5px; margin-top: 10px; }
    button { padding: 10px 15px; border: none; border-radius: 6px; background: #007bff; color: white; font-weight: bold; cursor: pointer; transition: 0.3s; flex: 1; text-align: center;}
    button:hover { background: #0056b3; }
    button.btn-danger { background: #dc3545; }
    button.btn-danger:hover { background: #c82333; }
    button.btn-success { background: #28a745; }
    button.btn-success:hover { background: #218838; }
    
    input[type="range"] { flex: 1; margin: 10px 0; width: 100%; cursor: pointer; }
    input[type="color"] { width: 60px; height: 40px; border: none; border-radius: 5px; cursor: pointer; background: transparent; }
  </style>
</head>
<body>
  <div class="container">
    <h2>🏠 YoloHome Dashboard</h2>
    
    <div class="card">
      <h3>📊 Biểu đồ Cảm biến Hiện tại</h3>
      <div id="pirStatus" style="text-align: center; font-size: 18px; font-weight: bold; margin-bottom: 15px;">--</div>
      
      <div class="chart-grid">
        <div class="chart-box">
          <div>Nhiệt độ: <span id="tVal" class="sensor-val" style="color:#dc3545">--</span>&deg;C</div>
          <canvas id="tChart"></canvas>
        </div>
        <div class="chart-box">
          <div>Độ ẩm: <span id="hVal" class="sensor-val" style="color:#0d6efd">--</span>%</div>
          <canvas id="hChart"></canvas>
        </div>
        <div class="chart-box">
          <div>Ánh sáng: <span id="lVal" class="sensor-val" style="color:#ffc107">--</span>%</div>
          <canvas id="lChart"></canvas>
        </div>
      </div>
    </div>

    <div class="card">
      <h3>🎛️ Bảng Điều khiển</h3>
      
      <div class="control-group">
        <div style="width: 100%; display: flex; justify-content: space-between; align-items: center;">
          <label><b>💡 Đèn RGB:</b> Chọn màu -> </label>
          <input type="color" id="rgbColor" value="#ff0000" onchange="pickColor(this.value)">
        </div>
        <div class="control-actions" style="width: 100%;">
          <button class="btn-success" onclick="rgbOn()">Bật Đèn</button>
          <button class="btn-danger" onclick="rgbOff()">Tắt Đèn</button>
          <button onclick="rgbAuto()">Chế độ Auto</button>
        </div>
      </div>

      <div class="control-group">
        <div style="width: 100%;">
          <label><b>❄️ Quạt gió:</b> Tốc độ <span id="fanVal" style="font-weight:bold; color:#007bff">0</span>%</label>
          <input type="range" id="fanSlider" min="0" max="100" value="0" onchange="setFan(this.value)">
        </div>
        <div class="control-actions" style="width: 100%;">
          <button class="btn-success" onclick="fanOn()">Bật Quạt (100%)</button>
          <button class="btn-danger" onclick="fanOff()">Tắt Quạt (0%)</button>
        </div>
      </div>

      <div class="control-group" style="background: transparent; padding: 0;">
        <button class="btn-success" onclick="openDoor()" style="width: 100%; font-size: 16px; padding: 15px;">🔓 MỞ CỬA CHÍNH</button>
      </div>
    </div>
  </div>

  <script>
    // --- KHỞI TẠO BIỂU ĐỒ ---
    const maxDataPoints = 15; // Lưu tối đa 15 điểm trên biểu đồ
    let timeData = [];
    let tempData = []; let humData = []; let lightData = [];

    const chartOpt = { 
      animation: false, 
      scales: { x: { display: false } }, // Ẩn trục X cho gọn
      plugins: { legend: { display: false } },
      maintainAspectRatio: false
    };

    function createChart(ctxId, color) {
      return new Chart(document.getElementById(ctxId).getContext('2d'), {
        type: 'line',
        data: { labels: timeData, datasets: [{ data: [], borderColor: color, borderWidth: 2, tension: 0.3, pointRadius: 0 }] },
        options: chartOpt
      });
    }

    const tChart = createChart('tChart', '#dc3545');
    const hChart = createChart('hChart', '#0d6efd');
    const lChart = createChart('lChart', '#ffc107');

    // --- HÀM CẬP NHẬT DỮ LIỆU ---
    function refreshSensors() {
      fetch('/api/sensors').then(r => r.json()).then(d => {
        // Cập nhật Text
        document.getElementById('tVal').textContent = d.temperature.toFixed(1);
        document.getElementById('hVal').textContent = d.humidity.toFixed(1);
        document.getElementById('lVal').textContent = d.light;
        
        let p = document.getElementById('pirStatus');
        if(d.pirDetected) { p.innerHTML = '🚨 <span style="color:red">Phát hiện Chuyển động!</span>'; } 
        else { p.innerHTML = '✅ <span style="color:#28a745">An toàn</span>'; }
        
        // Không đè giá trị slider nếu user đang kéo
        if (document.activeElement !== document.getElementById('fanSlider')) {
          document.getElementById('fanVal').textContent = d.fanSpeed;
          document.getElementById('fanSlider').value = d.fanSpeed;
        }

        // Cập nhật Biểu đồ
        let now = new Date();
        timeData.push(now.getSeconds());
        tempData.push(d.temperature);
        humData.push(d.humidity);
        lightData.push(d.light);

        if(timeData.length > maxDataPoints) {
          timeData.shift(); tempData.shift(); humData.shift(); lightData.shift();
        }

        tChart.update(); hChart.update(); lChart.update();

      }).catch(e => console.log("Lỗi tải dữ liệu", e));
    }

    // --- CÁC HÀM ĐIỀU KHIỂN ĐÈN ---
    function pickColor(hex) {
      let r = parseInt(hex.slice(1, 3), 16); let g = parseInt(hex.slice(3, 5), 16); let b = parseInt(hex.slice(5, 7), 16);
      fetch(`/api/rgb?r=${r}&g=${g}&b=${b}`);
    }
    function rgbOn() { pickColor(document.getElementById('rgbColor').value); } // Lấy màu hiện tại trên bảng màu để bật
    function rgbOff() { fetch('/api/rgb/off'); }
    function rgbAuto() { fetch('/api/rgb/auto'); }

    // --- CÁC HÀM ĐIỀU KHIỂN QUẠT ---
    function setFan(v) { 
      fetch(`/api/fan?speed=${v}`); 
      document.getElementById('fanVal').textContent = v; 
    }
    function fanOn() { setFan(100); document.getElementById('fanSlider').value = 100; }
    function fanOff() { setFan(0); document.getElementById('fanSlider').value = 0; }

    // --- ĐIỀU KHIỂN CỬA ---
    function openDoor() { fetch('/api/door'); alert('Đã gửi lệnh mở cửa!'); }

    // Chạy vòng lặp làm mới dữ liệu mỗi 2 giây
    setInterval(refreshSensors, 2000);
    refreshSensors();
  </script>
</body>
</html>
)rawliteral";

// =======================================================================
// CÁC HÀM XỬ LÝ API (Giữ nguyên, không cần thay đổi)
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
  doorState = DOOR_OPENING; 
  doorOpenTime = millis_present;
  
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Door Opened");
}

static void handleDashboardRoot()
{
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
  dashboardServer.on("/api/door", HTTP_GET, handleDoorOpen);

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