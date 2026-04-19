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
  <title>YoloHome Command Center</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@400;500;700&family=IBM+Plex+Sans:wght@400;500;600&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #09121d;
      --bg-soft: #111f2d;
      --panel: rgba(15, 30, 43, 0.86);
      --panel-border: rgba(255, 255, 255, 0.14);
      --text: #ebf3ff;
      --muted: #9db4cf;
      --teal: #1bc7b1;
      --orange: #ff9f5a;
      --red: #ff5a67;
      --lime: #6ce39e;
      --blue: #4aa2ff;
      --yellow: #ffd16a;
      --shadow: 0 20px 45px rgba(0, 0, 0, 0.28);
      --radius: 20px;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      font-family: 'IBM Plex Sans', sans-serif;
      color: var(--text);
      background:
        radial-gradient(70vw 80vh at 10% 5%, #163852 0%, transparent 70%),
        radial-gradient(90vw 90vh at 90% 100%, #2a1b2f 0%, transparent 70%),
        linear-gradient(160deg, #060d15 0%, #0a1522 45%, #0f1b2c 100%);
      overflow-x: hidden;
    }

    .orb {
      position: fixed;
      border-radius: 50%;
      filter: blur(10px);
      opacity: 0.35;
      z-index: -1;
      animation: drift 14s ease-in-out infinite;
    }

    .orb-one {
      width: 280px;
      height: 280px;
      background: #16d0b8;
      top: -80px;
      right: -50px;
    }

    .orb-two {
      width: 220px;
      height: 220px;
      background: #ff9f5a;
      left: -70px;
      bottom: 8vh;
      animation-delay: -4s;
    }

    .app {
      max-width: 1180px;
      margin: 0 auto;
      padding: 18px 14px 30px;
      display: grid;
      gap: 14px;
    }

    .panel {
      background: var(--panel);
      border: 1px solid var(--panel-border);
      border-radius: var(--radius);
      box-shadow: var(--shadow);
      backdrop-filter: blur(6px);
      padding: 16px;
    }

    .reveal {
      opacity: 0;
      transform: translateY(14px);
      animation: panelIn 0.55s ease forwards;
    }

    .reveal:nth-child(2) { animation-delay: 0.08s; }
    .reveal:nth-child(3) { animation-delay: 0.16s; }
    .reveal:nth-child(4) { animation-delay: 0.24s; }
    .reveal:nth-child(5) { animation-delay: 0.32s; }

    .hero {
      display: grid;
      gap: 12px;
      grid-template-columns: 1.5fr 1fr;
      align-items: stretch;
    }

    .title-wrap h1 {
      font-family: 'Space Grotesk', sans-serif;
      margin: 0;
      font-size: clamp(1.4rem, 2.4vw, 2rem);
      letter-spacing: 0.4px;
    }

    .title-wrap p {
      margin: 7px 0 0;
      color: var(--muted);
      font-size: 0.96rem;
    }

    .status-grid {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-top: 12px;
    }

    .badge {
      border-radius: 999px;
      padding: 7px 12px;
      font-size: 0.78rem;
      font-weight: 600;
      border: 1px solid transparent;
      background: rgba(255, 255, 255, 0.08);
      color: #d6e3f2;
      transition: all 0.25s ease;
    }

    .badge.online { background: rgba(108, 227, 158, 0.16); border-color: rgba(108, 227, 158, 0.34); color: #b9ffd4; }
    .badge.warn { background: rgba(255, 209, 106, 0.14); border-color: rgba(255, 209, 106, 0.34); color: #ffe7a5; }
    .badge.offline { background: rgba(255, 90, 103, 0.14); border-color: rgba(255, 90, 103, 0.32); color: #ffc2c8; }
    .badge.motion { background: rgba(255, 90, 103, 0.2); border-color: rgba(255, 90, 103, 0.44); color: #ffd4d8; }

    .hero-right {
      display: grid;
      align-content: center;
      justify-items: end;
      gap: 8px;
    }

    .clock {
      font-family: 'Space Grotesk', sans-serif;
      font-size: clamp(1.45rem, 2.6vw, 2.1rem);
      letter-spacing: 1.2px;
    }

    .last-sync {
      color: var(--muted);
      font-size: 0.88rem;
    }

    .kpi-grid {
      display: grid;
      grid-template-columns: repeat(6, minmax(140px, 1fr));
      gap: 10px;
    }

    .kpi {
      background: linear-gradient(155deg, rgba(255, 255, 255, 0.06), rgba(255, 255, 255, 0.02));
      border: 1px solid rgba(255, 255, 255, 0.09);
      border-radius: 16px;
      padding: 12px;
      display: grid;
      gap: 7px;
      min-height: 118px;
    }

    .kpi small {
      color: var(--muted);
      font-weight: 500;
      font-size: 0.78rem;
      text-transform: uppercase;
      letter-spacing: 0.8px;
    }

    .kpi .value {
      font-family: 'Space Grotesk', sans-serif;
      font-size: 1.35rem;
      font-weight: 700;
      line-height: 1;
    }

    .meter {
      width: 100%;
      height: 8px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.12);
      overflow: hidden;
    }

    .meter > span {
      display: block;
      height: 100%;
      width: 0;
      border-radius: inherit;
      transition: width 0.45s ease;
    }

    .meter.temp > span { background: linear-gradient(90deg, #ff7c5f, #ffb273); }
    .meter.hum > span { background: linear-gradient(90deg, #4aa2ff, #63d3ff); }
    .meter.light > span { background: linear-gradient(90deg, #ffd16a, #ffb44f); }
    .meter.fan > span { background: linear-gradient(90deg, #6ce39e, #2dc7b3); }

    .led-preview {
      width: 36px;
      height: 36px;
      border-radius: 12px;
      border: 2px solid rgba(255, 255, 255, 0.3);
      box-shadow: inset 0 0 12px rgba(255, 255, 255, 0.16), 0 0 16px rgba(0, 0, 0, 0.35);
      justify-self: start;
    }

    .charts {
      display: grid;
      grid-template-columns: repeat(3, minmax(180px, 1fr));
      gap: 10px;
    }

    .chart-box {
      background: linear-gradient(150deg, rgba(255,255,255,0.08), rgba(255,255,255,0.03));
      border: 1px solid rgba(255,255,255,0.1);
      border-radius: 16px;
      padding: 12px;
    }

    .chart-box h3 {
      margin: 0 0 4px;
      font-family: 'Space Grotesk', sans-serif;
      font-size: 0.98rem;
    }

    .chart-box p {
      margin: 0 0 8px;
      color: var(--muted);
      font-size: 0.8rem;
    }

    canvas {
      width: 100%;
      height: 130px !important;
    }

    .controls {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }

    .panel-title {
      margin: 0 0 10px;
      font-family: 'Space Grotesk', sans-serif;
      font-size: 1rem;
      letter-spacing: 0.35px;
    }

    .row {
      display: flex;
      align-items: center;
      gap: 8px;
      flex-wrap: wrap;
    }

    .spaced {
      justify-content: space-between;
    }

    .btn-grid {
      display: grid;
      grid-template-columns: repeat(3, minmax(80px, 1fr));
      gap: 8px;
      margin-top: 10px;
    }

    button {
      border: 0;
      border-radius: 12px;
      padding: 10px 12px;
      font-weight: 600;
      font-family: 'IBM Plex Sans', sans-serif;
      cursor: pointer;
      transition: transform 0.18s ease, filter 0.18s ease;
      color: #08111b;
      background: linear-gradient(140deg, #d7e8ff, #c3dbf8);
    }

    button:hover {
      transform: translateY(-1px);
      filter: brightness(1.05);
    }

    button:active {
      transform: translateY(0);
      filter: brightness(0.98);
    }

    .btn-ok { background: linear-gradient(140deg, #99f2cb, #72ddb1); }
    .btn-warn { background: linear-gradient(140deg, #ffd188, #ffb85c); }
    .btn-danger { background: linear-gradient(140deg, #ff9ea7, #ff7583); }
    .btn-teal { background: linear-gradient(140deg, #99f1e7, #53d8c8); }
    .btn-door { width: 100%; margin-top: 10px; font-size: 1rem; padding: 13px; }

    input[type='color'] {
      width: 56px;
      height: 40px;
      border: 0;
      border-radius: 12px;
      background: transparent;
      cursor: pointer;
    }

    input[type='range'] {
      width: 100%;
      accent-color: #4bd8c5;
    }

    .preset-grid {
      display: grid;
      grid-template-columns: repeat(4, minmax(58px, 1fr));
      gap: 8px;
      margin-top: 10px;
    }

    .preset {
      border-radius: 999px;
      height: 32px;
      border: 2px solid rgba(255, 255, 255, 0.45);
      cursor: pointer;
      transition: transform 0.16s ease;
    }

    .preset:hover { transform: scale(1.06); }

    .log-list {
      list-style: none;
      margin: 0;
      padding: 0;
      display: grid;
      gap: 7px;
      max-height: 155px;
      overflow: auto;
    }

    .log-list li {
      background: rgba(255, 255, 255, 0.07);
      border: 1px solid rgba(255, 255, 255, 0.09);
      border-radius: 10px;
      padding: 7px 10px;
      font-size: 0.86rem;
      color: #dce9f7;
    }

    .log-time {
      color: #9fc4e0;
      margin-right: 6px;
      font-weight: 600;
    }

    @keyframes panelIn {
      to {
        opacity: 1;
        transform: translateY(0);
      }
    }

    @keyframes drift {
      0% { transform: translate(0, 0); }
      50% { transform: translate(-8px, 10px); }
      100% { transform: translate(0, 0); }
    }

    @media (max-width: 980px) {
      .hero { grid-template-columns: 1fr; }
      .hero-right { justify-items: start; }
      .kpi-grid { grid-template-columns: repeat(3, minmax(130px, 1fr)); }
      .charts { grid-template-columns: 1fr; }
      .controls { grid-template-columns: 1fr; }
    }

    @media (max-width: 620px) {
      .app { padding: 12px 10px 18px; }
      .kpi-grid { grid-template-columns: repeat(2, minmax(120px, 1fr)); }
      .btn-grid { grid-template-columns: 1fr; }
      .preset-grid { grid-template-columns: repeat(4, minmax(48px, 1fr)); }
    }
  </style>
</head>
<body>
  <div class="orb orb-one"></div>
  <div class="orb orb-two"></div>

  <main class="app">
    <section class="panel hero reveal">
      <div class="title-wrap">
        <h1>YoloHome Command Center</h1>
        <p>Bang dieu khien thoi gian thuc cho cam bien, den RGB, quat va cua chinh.</p>
        <div class="status-grid">
          <span id="wifiBadge" class="badge">WiFi: --</span>
          <span id="mqttBadge" class="badge">MQTT: --</span>
          <span id="dhtBadge" class="badge">DHT20: --</span>
          <span id="pirBadge" class="badge">PIR: --</span>
        </div>
      </div>
      <div class="hero-right">
        <div id="clock" class="clock">--:--:--</div>
        <div id="lastSync" class="last-sync">Dang cho du lieu...</div>
      </div>
    </section>

    <section class="panel reveal">
      <div class="kpi-grid">
        <article class="kpi">
          <small>Nhiet do</small>
          <div id="tVal" class="value">--.- C</div>
          <div class="meter temp"><span id="tempBar"></span></div>
        </article>
        <article class="kpi">
          <small>Do am</small>
          <div id="hVal" class="value">--.- %</div>
          <div class="meter hum"><span id="humBar"></span></div>
        </article>
        <article class="kpi">
          <small>Anh sang</small>
          <div id="lVal" class="value">-- %</div>
          <div class="meter light"><span id="lightBar"></span></div>
        </article>
        <article class="kpi">
          <small>Toc do quat</small>
          <div id="fanSpeedVal" class="value">-- %</div>
          <div class="meter fan"><span id="fanBar"></span></div>
        </article>
        <article class="kpi">
          <small>RGB mode</small>
          <div id="ledModeVal" class="value">--</div>
          <div class="row">
            <div id="ledPreview" class="led-preview"></div>
            <span id="ledHex" style="color:var(--muted); font-size:0.88rem;">#------</span>
          </div>
        </article>
        <article class="kpi">
          <small>Cua tu dong</small>
          <div id="doorStatusVal" class="value">--</div>
          <span id="doorStatusMeta" style="color:var(--muted); font-size:0.86rem;">state=--</span>
        </article>
        <article class="kpi">
          <small>Uptime</small>
          <div id="uptimeVal" class="value">--</div>
          <span style="color:var(--muted); font-size:0.86rem;">thiet bi dang hoat dong</span>
        </article>
      </div>
    </section>

    <section class="panel reveal">
      <div class="charts">
        <div class="chart-box">
          <h3>Nhip nhiet do</h3>
          <p>Xu huong 24 mau gan nhat</p>
          <canvas id="tChart"></canvas>
        </div>
        <div class="chart-box">
          <h3>Nhip do am</h3>
          <p>Bien thien moi truong</p>
          <canvas id="hChart"></canvas>
        </div>
        <div class="chart-box">
          <h3>Nhip anh sang</h3>
          <p>Cuong do theo thoi gian</p>
          <canvas id="lChart"></canvas>
        </div>
      </div>
    </section>

    <section class="controls reveal">
      <div class="panel">
        <h2 class="panel-title">Lighting Studio</h2>
        <div class="row spaced">
          <label for="rgbColor">Mau thu cong</label>
          <input type="color" id="rgbColor" value="#ff7a5e" onchange="pickColor(this.value)">
        </div>

        <div class="btn-grid">
          <button class="btn-ok" onclick="rgbOn()">Bat</button>
          <button class="btn-danger" onclick="rgbOff()">Tat</button>
          <button class="btn-teal" onclick="rgbAuto()">Auto</button>
        </div>

        <div class="preset-grid">
          <button class="preset" style="background:#ff5d6f" onclick="applyPreset('#FF5D6F')" aria-label="Rose"></button>
          <button class="preset" style="background:#ffad57" onclick="applyPreset('#FFAD57')" aria-label="Sunset"></button>
          <button class="preset" style="background:#3ddc97" onclick="applyPreset('#3DDC97')" aria-label="Mint"></button>
          <button class="preset" style="background:#58a7ff" onclick="applyPreset('#58A7FF')" aria-label="Sky"></button>
        </div>

        <p style="margin:10px 0 0; color:var(--muted); font-size:0.86rem;">Preset chi doi mau khi den dang bat.</p>
      </div>

      <div class="panel">
        <h2 class="panel-title">Climate and Access</h2>
        <div class="row spaced">
          <label for="fanSlider">Toc do quat: <span id="fanValInline" style="font-weight:700;">0</span>%</label>
          <span style="color:var(--muted); font-size:0.84rem;">PWM Control</span>
        </div>
        <input type="range" id="fanSlider" min="0" max="100" value="0" oninput="updateFanPreview(this.value)" onchange="setFan(this.value)">

        <div class="btn-grid">
          <button class="btn-ok" onclick="fanOn()">Quat 100%</button>
          <button class="btn-warn" onclick="setFan(60)">Quat 60%</button>
          <button class="btn-danger" onclick="fanOff()">Quat 0%</button>
        </div>

        <div class="row spaced" style="margin-top:12px;">
          <span>Trang thai cua: <strong id="doorStatusInline">--</strong></span>
          <span style="color:var(--muted); font-size:0.84rem;">Servo GPIO48</span>
        </div>

        <div class="btn-grid">
          <button class="btn-warn" onclick="openDoor()">Mo cua</button>
          <button class="btn-teal" onclick="closeDoor()">Dong cua</button>
          <button onclick="refreshSensors()">Lam moi</button>
        </div>
      </div>
    </section>

    <section class="panel reveal">
      <h2 class="panel-title">Event Timeline</h2>
      <ul id="eventLog" class="log-list"></ul>
    </section>
  </main>

  <script>
    const maxDataPoints = 24;
    let timeData = [];
    let tempData = [];
    let humData = [];
    let lightData = [];

    function clockTick() {
      const now = new Date();
      document.getElementById('clock').textContent = now.toLocaleTimeString('vi-VN', { hour12: false });
    }

    function formatUptime(sec) {
      const s = Math.max(0, Number(sec) || 0);
      const h = Math.floor(s / 3600);
      const m = Math.floor((s % 3600) / 60);
      const r = Math.floor(s % 60);
      return String(h).padStart(2, '0') + ':' + String(m).padStart(2, '0') + ':' + String(r).padStart(2, '0');
    }

    function toHex(v) {
      const n = Math.max(0, Math.min(255, Number(v) || 0));
      return n.toString(16).padStart(2, '0').toUpperCase();
    }

    function updateMeter(id, value, max) {
      const pct = Math.max(0, Math.min(100, (Number(value) || 0) * 100 / max));
      document.getElementById(id).style.width = pct + '%';
    }

    function setBadge(id, text, cssClass) {
      const el = document.getElementById(id);
      el.textContent = text;
      el.className = 'badge ' + cssClass;
    }

    function addLog(message) {
      const now = new Date();
      const t = now.toLocaleTimeString('vi-VN', { hour12: false });
      const li = document.createElement('li');
      li.innerHTML = '<span class="log-time">' + t + '</span>' + message;
      const log = document.getElementById('eventLog');
      log.prepend(li);
      while (log.children.length > 12) {
        log.removeChild(log.lastChild);
      }
    }

    const chartOpt = {
      animation: false,
      scales: {
        x: { display: false },
        y: {
          ticks: { color: 'rgba(219,233,248,0.68)', font: { size: 10 } },
          grid: { color: 'rgba(255,255,255,0.08)' }
        }
      },
      plugins: { legend: { display: false } },
      maintainAspectRatio: false
    };

    function createChart(ctxId, color, dataArray) {
      return new Chart(document.getElementById(ctxId).getContext('2d'), {
        type: 'line',
        data: {
          labels: timeData,
          datasets: [{
            data: dataArray,
            borderColor: color,
            borderWidth: 2,
            tension: 0.34,
            pointRadius: 0,
            fill: true,
            backgroundColor: color + '22'
          }]
        },
        options: chartOpt
      });
    }

    const tChart = createChart('tChart', '#ff8a63', tempData);
    const hChart = createChart('hChart', '#5ec2ff', humData);
    const lChart = createChart('lChart', '#ffcc66', lightData);

    function applyPreset(hex) {
      document.getElementById('rgbColor').value = hex;
      pickColor(hex);
      addLog('Doi mau preset ' + hex);
    }

    function updateFanPreview(v) {
      document.getElementById('fanValInline').textContent = v;
    }

    function refreshSensors() {
      fetch('/api/sensors', { cache: 'no-store' }).then(r => r.json()).then(d => {
        const temp = Number(d.temperature) || 0;
        const hum = Number(d.humidity) || 0;
        const light = Number(d.light) || 0;
        const fan = Number(d.fanSpeed) || 0;
        const doorState = Number(d.doorState);
        const doorStatus = String(d.doorStatus || 'unknown').toUpperCase();
        const ledHex = '#' + toHex(d.ledR) + toHex(d.ledG) + toHex(d.ledB);
        const mode = d.ledMode || 'off';

        document.getElementById('tVal').textContent = temp.toFixed(1) + ' C';
        document.getElementById('hVal').textContent = hum.toFixed(1) + ' %';
        document.getElementById('lVal').textContent = Math.round(light) + ' %';
        document.getElementById('fanSpeedVal').textContent = Math.round(fan) + ' %';
        document.getElementById('fanValInline').textContent = Math.round(fan);
        document.getElementById('ledModeVal').textContent = String(mode).toUpperCase();
        document.getElementById('ledHex').textContent = ledHex;
        document.getElementById('ledPreview').style.background = d.ledEnabled ? ledHex : '#0f1722';
        document.getElementById('doorStatusVal').textContent = doorStatus;
        document.getElementById('doorStatusMeta').textContent = Number.isFinite(doorState) ? ('state=' + doorState) : 'state=--';
        document.getElementById('doorStatusInline').textContent = doorStatus;
        document.getElementById('uptimeVal').textContent = formatUptime(d.uptimeSec);

        updateMeter('tempBar', temp, 50);
        updateMeter('humBar', hum, 100);
        updateMeter('lightBar', light, 100);
        updateMeter('fanBar', fan, 100);

        if (document.activeElement !== document.getElementById('fanSlider')) {
          document.getElementById('fanSlider').value = Math.round(fan);
        }

        setBadge('wifiBadge', 'WiFi: online', 'online');
        setBadge('mqttBadge', d.mqttConnected ? 'MQTT: connected' : 'MQTT: disconnected', d.mqttConnected ? 'online' : 'offline');
        setBadge('dhtBadge', d.dhtValid ? 'DHT20: healthy' : 'DHT20: error', d.dhtValid ? 'online' : 'warn');
        setBadge('pirBadge', d.pirDetected ? 'PIR: motion detected' : 'PIR: safe', d.pirDetected ? 'motion' : 'online');

        const now = new Date();
        document.getElementById('lastSync').textContent = 'Dong bo luc ' + now.toLocaleTimeString('vi-VN', { hour12: false });

        const timeStr = now.toLocaleTimeString('vi-VN', { hour12: false });
        timeData.push(timeStr);
        tempData.push(temp);
        humData.push(hum);
        lightData.push(light);

        if (timeData.length > maxDataPoints) {
          timeData.shift();
          tempData.shift();
          humData.shift();
          lightData.shift();
        }

        tChart.update();
        hChart.update();
        lChart.update();
      }).catch(() => {
        setBadge('wifiBadge', 'WiFi: unavailable', 'offline');
      });
    }

    function pickColor(hex) {
      const r = parseInt(hex.slice(1, 3), 16);
      const g = parseInt(hex.slice(3, 5), 16);
      const b = parseInt(hex.slice(5, 7), 16);
      fetch('/api/rgb?r=' + r + '&g=' + g + '&b=' + b);
      addLog('RGB set ' + hex.toUpperCase());
    }

    function rgbOn() {
      pickColor(document.getElementById('rgbColor').value);
      addLog('Bat den RGB');
    }

    function rgbOff() {
      fetch('/api/rgb/off');
      addLog('Tat den RGB');
    }

    function rgbAuto() {
      fetch('/api/rgb/auto');
      addLog('Bat che do auto RGB');
    }

    function setFan(v) {
      const speed = Math.max(0, Math.min(100, Number(v) || 0));
      fetch('/api/fan?speed=' + speed);
      document.getElementById('fanValInline').textContent = speed;
      addLog('Dat toc do quat ' + speed + '%');
    }

    function fanOn() {
      document.getElementById('fanSlider').value = 100;
      setFan(100);
    }

    function fanOff() {
      document.getElementById('fanSlider').value = 0;
      setFan(0);
    }

    function openDoor() {
      fetch('/api/door/open').then(() => refreshSensors());
      addLog('Gui lenh mo cua chinh');
    }

    function closeDoor() {
      fetch('/api/door/close').then(() => refreshSensors());
      addLog('Gui lenh dong cua chinh');
    }

    addLog('Khoi dong dashboard');
    clockTick();
    setInterval(clockTick, 1000);
    setInterval(refreshSensors, 2000);
    refreshSensors();
  </script>
</body>
</html>
)rawliteral";

static void handleApiSensors()
{
  JsonDocument doc;
  const char* ledMode = "off";
  if (mqttLedState) {
    ledMode = led_rgb_is_auto() ? "auto" : "manual";
  }

  doc["temperature"] = Value_Temperature;
  doc["humidity"] = Value_Humidity;
  doc["light"] = Value_Light;
  doc["pirDetected"] = pirDetected;
  doc["dhtValid"] = dhtDataValid;
  doc["mqttConnected"] = isMqttConnected();
  doc["ledEnabled"] = mqttLedState;
  doc["ledR"] = mqttLedR;
  doc["ledG"] = mqttLedG;
  doc["ledB"] = mqttLedB;
  doc["ledMode"] = ledMode;
  doc["fanSpeed"] = mqttFanSpeed;
  doc["doorState"] = (int)doorState;
  doc["doorStatus"] = door_state_to_text(doorState);
  doc["uptimeSec"] = millis_present / 1000;

  String payload;
  serializeJson(doc, payload);

  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "application/json", payload);
}

static void handleRgbSet()
{
  int r = dashboardServer.hasArg("r") ? constrain(dashboardServer.arg("r").toInt(), 0, 255) : 255;
  int g = dashboardServer.hasArg("g") ? constrain(dashboardServer.arg("g").toInt(), 0, 255) : 255;
  int b = dashboardServer.hasArg("b") ? constrain(dashboardServer.arg("b").toInt(), 0, 255) : 255;

  mqttLedState = true;
  mqttLedR = r;
  mqttLedG = g;
  mqttLedB = b;
  led_rgb_set_auto(false);
  led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
  publishActuatorStatus();

  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "RGB set");
}

static void handleRgbOff()
{
  mqttLedState = false;
  led_rgb_set_auto(false);
  led_rgb_off();
  publishActuatorStatus();

  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "RGB off");
}

static void handleRgbAuto()
{
  mqttLedState = true;
  led_rgb_set_auto(true);
  publishActuatorStatus();

  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "RGB auto");
}

static void handleFanSet()
{
  if (dashboardServer.hasArg("speed")) {
    mqttFanSpeed = constrain(dashboardServer.arg("speed").toInt(), 0, 100);
    fan_set_speed(mqttFanSpeed);
    publishActuatorStatus();
  }
  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Fan set");
}

static void handleDoorOpen()
{
  door_command_open();
  publishActuatorStatus();

  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Door Opened");
}

static void handleDoorClose()
{
  door_command_close();
  publishActuatorStatus();

  dashboardServer.sendHeader("Access-Control-Allow-Origin", "*");
  dashboardServer.send(200, "text/plain", "Door Closed");
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
  dashboardServer.on("/api/door/open", HTTP_GET, handleDoorOpen);
  dashboardServer.on("/api/door/close", HTTP_GET, handleDoorClose);
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
