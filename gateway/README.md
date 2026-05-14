# Huong dan test YoloHome IoT Gateway

Tai lieu nay huong dan test luong Store-and-Forward:

```text
ESP32-S3 -> Mosquitto local -> Python Gateway -> HiveMQ Cloud
                              |
                              +-> SQLite buffer khi mat Internet/Cloud
```

Gateway dung de ESP32 van gui du lieu trong LAN, con laptop/Raspberry Pi chiu trach nhiem forward len HiveMQ Cloud. Khi Cloud mat ket noi, Gateway luu ban tin vao SQLite va gui bu khi ket noi phuc hoi.

## 1. Chuan bi

Can co:

- ESP32-S3/Yolo Uno da cam USB.
- Laptop va ESP32 cung mot WiFi.
- Mosquitto MQTT broker tren laptop.
- Python virtualenv cho Gateway.
- HiveMQ Cloud credentials neu muon test forward len Cloud that.

Kiem tra IP laptop:

```powershell
ipconfig
```

Lay dong `Wireless LAN adapter Wi-Fi` -> `IPv4 Address`. Vi du:

```text
192.168.100.139
```

Day la IP laptop/Mosquitto, khong phai IP ESP32.

## 2. Cai va cau hinh Mosquitto local

Neu chua cai Mosquitto:

```powershell
winget install -e --id EclipseFoundation.Mosquitto
```

Mo PowerShell bang quyen Administrator, sua config:

```powershell
notepad "C:\Program Files\Mosquitto\mosquitto.conf"
```

Them cuoi file:

```conf
listener 1883 0.0.0.0
allow_anonymous true
```

Restart service:

```powershell
Restart-Service mosquitto
```

Mo firewall cho ESP32 ket noi vao laptop:

```powershell
New-NetFirewallRule -DisplayName "Mosquitto MQTT 1883" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow -Profile Any
```

Kiem tra Mosquitto da listen LAN:

```powershell
netstat -ano | findstr :1883
```

Ket qua dung can co:

```text
0.0.0.0:1883 LISTENING
```

Kiem tra TCP:

```powershell
Test-NetConnection -ComputerName 192.168.100.139 -Port 1883
```

Ket qua dung:

```text
TcpTestSucceeded : True
```

## 3. Cau hinh firmware ESP32 de gui ve Mosquitto local

Mo `platformio.ini`, env `yolo_uno_gateway` can tro ve IP laptop:

```ini
[env:yolo_uno_gateway]
extends = env:yolo_uno
build_flags =
  ${env:yolo_uno.build_flags}
  -D MQTT_SERVER=\"192.168.100.139\"
  -D MQTT_PORT=1883
  -D MQTT_USE_TLS=0
  -D MQTT_USE_AUTH=0
```

Neu doi WiFi, IP laptop co the doi. Chay lai `ipconfig` va cap nhat `MQTT_SERVER`.

## 4. Upload firmware Gateway mode

Vao dung thu muc project:

```powershell
cd "D:\Github\Smart Home IOT Firmware"
```

Kiem tra COM port:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe device list
```

Upload:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe run -e yolo_uno_gateway -t upload
```

Neu can chi ro COM:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe run -e yolo_uno_gateway -t upload --upload-port COM9
```

Mo Serial Monitor:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe device monitor -b 115200
```

Log dung:

```text
WiFi connected
MQTT TLS disabled: using plain TCP for local gateway broker
MQTT target: 192.168.100.139:1883 tls=0 auth=0
Connecting MQTT...MQTT connected
Published structured state payload
Registered MQTT channels: cmd
```

Neu thay `Failed, rc=-2`, ESP32 chua ket noi duoc TCP toi Mosquitto. Kiem tra lai IP, firewall, Mosquitto service, va router co bat client isolation hay khong.

## 5. Test ESP32 publish len MQTT local

Mo PowerShell moi de subscribe:

```powershell
& "C:\Program Files\Mosquitto\mosquitto_sub.exe" -h 192.168.100.139 -p 1883 -t "yolohome/device/yolo_uno_01/#" -v
```

Reset ESP32. Ban se thay cac ban tin:

```text
yolohome/device/yolo_uno_01/availability online
yolohome/device/yolo_uno_01/state {...}
yolohome/device/yolo_uno_01/telemetry {...}
```

Telemetry gui theo chu ky khoang 30 giay.

## 6. Test gui lenh dieu khien tu MQTT local ve ESP32

Mo PowerShell khac, gui lenh quat 60%:

```powershell
& "C:\Program Files\Mosquitto\mosquitto_pub.exe" -h 192.168.100.139 -p 1883 -t "yolohome/device/yolo_uno_01/cmd" -m '{"commandId":"test-fan-1","target":"fan","action":"set","speed":60}'
```

Gui lenh LED mau do:

```powershell
& "C:\Program Files\Mosquitto\mosquitto_pub.exe" -h 192.168.100.139 -p 1883 -t "yolohome/device/yolo_uno_01/cmd" -m '{"commandId":"test-led-1","target":"led","action":"set","color":"red"}'
```

Tat LED:

```powershell
& "C:\Program Files\Mosquitto\mosquitto_pub.exe" -h 192.168.100.139 -p 1883 -t "yolohome/device/yolo_uno_01/cmd" -m '{"commandId":"test-led-off","target":"led","action":"off"}'
```

Ket qua dung trong cua so `mosquitto_sub`:

```text
yolohome/device/yolo_uno_01/ack {...}
yolohome/device/yolo_uno_01/state {...}
```

## 7. Test dashboard noi bo cua ESP32

Tu log Serial, lay IP cua ESP32. Vi du:

```text
IP Address: 192.168.100.30
```

Mo trinh duyet:

```text
http://192.168.100.30
```

Neu trang dashboard hien sensor/actuator la web dashboard noi bo hoat dong.

## 8. Chuan bi Python Gateway

Trong thu muc project:

```powershell
cd "D:\Github\Smart Home IOT Firmware"
```

Tao virtualenv va cai dependency:

```powershell
python -m venv gateway\.venv
gateway\.venv\Scripts\python.exe -m pip install -r gateway\requirements.txt
```

Tao file config rieng:

```powershell
Copy-Item gateway\config.example.json gateway\config.json
notepad gateway\config.json
```

Cau hinh toi thieu:

```json
{
  "database": "gateway_buffer.db",
  "local": {
    "host": "127.0.0.1",
    "port": 1883,
    "tls": false
  },
  "cloud": {
    "host": "YOUR_CLUSTER.s1.eu.hivemq.cloud",
    "port": 8883,
    "username": "YOUR_HIVEMQ_USERNAME",
    "password": "YOUR_HIVEMQ_PASSWORD",
    "tls": true,
    "insecure_tls": false
  }
}
```

Neu Gateway va Mosquitto chay cung laptop, `local.host` nen de `127.0.0.1`. File `gateway/config.json` da duoc ignore de tranh commit credential.

## 9. Chay Python Gateway

```powershell
gateway\.venv\Scripts\python.exe gateway\store_forward_gateway.py --config gateway\config.json --log-level INFO
```

Khi ESP32 publish len Mosquitto, Gateway se log cac dong dang:

```text
subscribed local uplink yolohome/device/yolo_uno_01/telemetry
cloud mqtt connected; pending=0
forwarded live yolohome/device/yolo_uno_01/telemetry
```

Mo HiveMQ Web Client hoac MQTT client va subscribe:

```text
yolohome/device/yolo_uno_01/#
```

Neu dung, ban se thay telemetry/state/event tu ESP32 len HiveMQ Cloud.

## 10. Test Store-and-Forward offline buffer

Muc tieu: chung minh ESP32 van gui vao Mosquitto local, Gateway luu SQLite khi Cloud mat ket noi.

1. Lam Cloud fail tam thoi, vi du sua sai `cloud.host` trong `gateway/config.json`.
2. Chay lai Gateway.
3. De ESP32 gui vai ban tin telemetry, hoac gui gia lap:

```powershell
& "C:\Program Files\Mosquitto\mosquitto_pub.exe" -h 192.168.100.139 -p 1883 -t "yolohome/device/yolo_uno_01/telemetry" -m '{"schemaVersion":1,"deviceId":"yolo_uno_01","ts":1,"temperature":29.5}'
```

4. Kiem tra SQLite queue.

Neu co `sqlite3`:

```powershell
sqlite3 gateway\gateway_buffer.db "select count(*), min(id), max(id) from outbound_queue;"
```

Neu khong co `sqlite3`, dung Python:

```powershell
gateway\.venv\Scripts\python.exe -c "import sqlite3; c=sqlite3.connect('gateway/gateway_buffer.db'); print(c.execute('select count(*), min(id), max(id) from outbound_queue').fetchone())"
```

Ket qua dung: `count` tang len khi Cloud offline.

5. Sua lai `cloud.host`, `username`, `password` dung.
6. Chay lai Gateway.
7. Queue se duoc flush len Cloud.

Kiem tra queue da rong:

```powershell
gateway\.venv\Scripts\python.exe -c "import sqlite3; c=sqlite3.connect('gateway/gateway_buffer.db'); print(c.execute('select count(*) from outbound_queue').fetchone()[0])"
```

Ket qua dung:

```text
0
```

## 11. Loi thuong gap

`Connecting MQTT...Failed, rc=-2` tren ESP32:

- Sai `MQTT_SERVER`.
- Mosquitto chua chay.
- Port `1883` chua listen `0.0.0.0`.
- Windows Firewall chua mo inbound TCP 1883.
- ESP32 va laptop khong cung WiFi.
- Router bat client isolation.

`NotPlatformIOProjectError`:

- Dang chay lenh o sai thu muc.
- Dung:

```powershell
cd "D:\Github\Smart Home IOT Firmware"
```

Hoac them `-d`:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe run -d "D:\Github\Smart Home IOT Firmware" -e yolo_uno_gateway -t upload
```

`Could not open COMx`:

- Sai COM port.
- Serial Monitor/Arduino IDE dang giu port.
- Board doi COM sau khi reset.
- Can vao bootloader thu cong: giu `BOOT`, bam/thả `RST/EN`, roi tha `BOOT`.

`WebServer request handler not found`:

- Thuong do browser goi `/favicon.ico` hoac route khong ton tai.
- Khong anh huong MQTT/Gateway.

`Preferences getString adminPass NOT_FOUND`:

- Lan dau flash chua co password trong NVS.
- Firmware se dung password mac dinh `123`.
