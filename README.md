# YoloHome AIoT Firmware Specification

Ngày rà soát: 2026-05-12

## 1. Mục tiêu dự án

Firmware điều khiển bộ Smart Home AIoT trên board ESP32-S3 `yolo_uno`, dùng Arduino framework qua PlatformIO. Hệ thống đọc cảm biến môi trường, điều khiển đèn RGB NeoPixel, quạt PWM, servo cửa, nhận mật khẩu qua IR remote, hiển thị LCD 16x2, cung cấp web dashboard nội bộ và đồng bộ trạng thái qua MQTT.

## 2. Môi trường build

- PlatformIO environment: `yolo_uno`
- Platform: `espressif32`
- Board: `yolo_uno` / `Yolo_Uno_S3`
- Framework: Arduino
- Serial monitor: `115200`
- Build flags:
  - `ARDUINO_USB_MODE=1`
  - `ARDUINO_USB_CDC_ON_BOOT=1`
- Thư viện chính:
  - `Adafruit NeoPixel`
  - `DHT20`
  - `LiquidCrystal_I2C`
  - `PubSubClient`
  - `ArduinoJson`
  - `ESP32Servo`
  - `IRremoteESP8266`
  - `Adafruit BMP280/BME280` hiện có trong dependencies nhưng source hiện tại chưa dùng trực tiếp.

## 3. Phần cứng và chân kết nối

| Thành phần | Chân / cấu hình | Ghi chú |
| --- | --- | --- |
| Cảm biến ánh sáng | `GPIO1` (`light`) | Đọc analog, map về 0..100%. |
| Quạt | `GPIO10` (`FAN_PIN`) | Điều khiển PWM bằng `analogWrite`. |
| NeoPixel RGB | `GPIO18` (`PIN_NEO_PIXEL`) | 4 LED, chuẩn `NEO_GRB + NEO_KHZ800`. |
| PIR | `GPIO7` (`PIR_PIN`) | Đọc chuyển động digital. |
| IR receiver | `GPIO9` (`IR_RECV_PIN`) | Dùng remote để nhập mật khẩu. |
| Servo cửa | `GPIO48` (`SERVO_PIN`) | Góc đóng 0 độ, mở 90 độ. |
| I2C LCD + DHT20 | SDA `GPIO11`, SCL `GPIO12` | LCD 16x2, DHT20 thường ở địa chỉ `0x38`. |
| LCD | 16 cột x 2 dòng | Tự dò địa chỉ ưu tiên `0x27`, `0x3F`, `0x20`, `0x26`. |

Lưu ý: comment trong code đang ghi NeoPixel là `GPIO6`, nhưng macro thực tế là `GPIO18`. Spec này ưu tiên giá trị macro đang được build.

## 4. Cấu hình định danh

- Device ID: `yolo_uno_01`
- MQTT schema version: `1`
- Firmware version: `2026.04`
- MQTT namespace: `yolohome/device/yolo_uno_01/`
- NTP timezone: GMT+7

Thông tin WiFi/MQTT hiện đang hard-code trong `lib/global_var/global_var.cpp`.

## 5. Luồng khởi động

`setup()` thực hiện tuần tự:

1. Khởi động Serial ở `115200`.
2. Cấu hình GPIO và I2C qua `pinSetup()`.
3. Scan I2C bus.
4. Khởi tạo NeoPixel, clear toàn bộ LED.
5. Dò địa chỉ và khởi tạo LCD.
6. Kết nối WiFi và cấu hình MQTT.
7. Đồng bộ NTP GMT+7.
8. Khởi tạo DHT20.
9. Khởi tạo servo ở trạng thái đóng.
10. Khởi tạo IR receiver và nạp mật khẩu từ flash.
11. Đăng ký MQTT channel.
12. Khởi động web dashboard trên port 80 nếu WiFi đã kết nối.
13. Đọc cảm biến lần đầu và hiển thị LCD.
14. Tạo các FreeRTOS task.

`loop()` không xử lý logic nghiệp vụ, chỉ delay 1 giây vì ứng dụng chạy bằng FreeRTOS task.

## 6. FreeRTOS task

| Task | Chu kỳ | Core | Chức năng |
| --- | ---: | ---: | --- |
| `task_comms` | 20 ms | 0 | Cập nhật `millis_present`, chạy MQTT loop, xử lý HTTP dashboard client. |
| `task_control` | 100 ms | 1 | LED auto cycle, đọc PIR, xử lý IR remote, PIR auto light nếu được bật cấu hình, servo cửa, timeout FSM mật khẩu. |
| `task_ui` | 1 s | 1 | Render dữ liệu lên LCD. |
| `task_telemetry` | 30 s | 1 | Đọc cảm biến và publish telemetry/state MQTT. |

## 7. Cảm biến

### DHT20

- Đọc nhiệt độ và độ ẩm qua I2C.
- Nếu đọc thành công:
  - `Value_Temperature`: làm tròn 2 chữ số thập phân.
  - `Value_Humidity`: làm tròn 2 chữ số thập phân.
  - `dhtDataValid = true`.
- Nếu lỗi:
  - `dhtDataValid = false`.
  - In log `DHT20 read failed`.

### Cảm biến ánh sáng

- Đọc analog từ `GPIO1`.
- Quy đổi:
  - raw `500` tương ứng 0%.
  - raw `4095` tương ứng 100%.
- Kết quả được constrain trong 0..100.

### PIR

- Đọc digital từ `GPIO7`.
- Cập nhật `pirDetected`.

## 8. Điều khiển thiết bị

### Quạt

- `fan_set_speed(speed)` nhận 0..100%.
- Firmware constrain speed về 0..100.
- PWM output được map về 0..255 trên `FAN_PIN`.

### NeoPixel RGB

- Có 4 pixel, mọi thao tác màu áp dụng cho toàn bộ 4 pixel.
- Chế độ manual:
  - Set RGB bằng giá trị `r/g/b`, hex hoặc tên màu.
  - Các màu tên hỗ trợ: `red`, `orange`, `yellow`, `green`, `cyan`, `blue`, `purple`, `white`, `off`.
- Chế độ auto:
  - Đổi màu mỗi 2 giây.
  - Vòng màu: đỏ, cam, vàng, xanh lá, cyan, xanh dương, tím, trắng.
- Chế độ off:
  - Tắt NeoPixel và tắt auto cycle.
  - PIR không tự bật LED khi `PIR_AUTO_LIGHT_ENABLED` đang để `0`.

### PIR auto light

- Mặc định đang tắt bằng cấu hình `PIR_AUTO_LIGHT_ENABLED 0` trong `lib/global_var/global_var.hpp`.
- Khi tắt cấu hình này, PIR chỉ cập nhật `pirDetected` để test/hiển thị, không tự bật đèn.
- Nếu đổi `PIR_AUTO_LIGHT_ENABLED` thành `1`, tính năng chỉ hoạt động khi không có điều khiển LED manual qua MQTT/dashboard (`mqttLedState == false`).
- Khi PIR phát hiện chuyển động: bật LED trắng.
- Khi hết chuyển động: giữ sáng 5 giây rồi tắt.

### Servo cửa

- Góc mở: 90 độ.
- Góc đóng: 0 độ.
- `door_command_open()`:
  - Gắn servo nếu cần.
  - Ghi góc mở.
  - Đặt `doorState = DOOR_OPENING`.
  - Sau 2 giây đặt `DOOR_OPEN` và giữ servo ở góc mở cho đến khi có lệnh đóng.
- `door_command_close()`:
  - Ghi góc đóng.
  - Đặt `doorState = DOOR_CLOSING`.
  - Sau 2 giây detach servo và đặt `DOOR_CLOSED`.
- Trạng thái:
  - `0 = closed`
  - `1 = opening`
  - `2 = closing`
  - `3 = open`

## 9. FSM mật khẩu qua IR remote

### Phím IR

Remote map các phím số `0..9`, phím `F` là Enter/OK, phím `C` là xóa lùi.

### Mật khẩu

- Mật khẩu mặc định: `123`.
- Mật khẩu được lưu trong flash bằng `Preferences`, namespace `door-lock`, key `adminPass`.
- Khi khởi động, firmware nạp mật khẩu đã lưu; nếu chưa có thì dùng `123`.

### Chế độ kiểm tra mật khẩu

- Nhập số rồi bấm `F`.
- Nếu input bằng mật khẩu hiện tại:
  - Publish feedback `Mat ma dung - Mo cua`.
  - Mở cửa bằng servo.
- Nếu input bằng mật khẩu lặp đôi, ví dụ mật khẩu `123` thì nhập `123123`:
  - Chuyển sang chế độ đổi mật khẩu.
  - Publish feedback yêu cầu nhập mật khẩu mới.
- Nếu sai:
  - Publish feedback `Sai mat ma!`.
  - Xóa input.

### Chế độ đổi mật khẩu

- Nhập mật khẩu mới tối đa 10 chữ số.
- Bấm `F` để lưu vào flash.
- Nếu bấm `F` khi chưa nhập gì thì hủy đổi mật khẩu.

### Timeout

- Nếu đang nhập hoặc đang ở chế độ đổi mật khẩu mà quá 10 giây không có phím:
  - Xóa input.
  - Trở về chế độ kiểm tra mật khẩu.
  - Publish feedback timeout.

## 10. LCD 16x2

LCD tự chuyển trang mỗi 5 giây, render tối đa mỗi 1 giây.

| Trang | Dòng 1 | Dòng 2 |
| --- | --- | --- |
| 1 | Nhiệt độ hoặc `DHT20 Error` | Độ ẩm hoặc `Check wiring` |
| 2 | Ánh sáng `%` | Trống |
| 3 | Trạng thái PIR | `Sensor page 3/3` |

## 11. MQTT

### Kết nối

- Broker: HiveMQ Cloud qua port `8883`.
- Client: `WiFiClientSecure` + `PubSubClient`.
- TLS hiện dùng `espClient.setInsecure()`, tức không xác thực certificate.
- LWT:
  - Topic: `yolohome/device/yolo_uno_01/availability`
  - Payload khi mất kết nối bất ngờ: `offline`
  - QoS: 1
  - Retain: true
- Khi reconnect thành công:
  - Subscribe topic command.
  - Publish `online`.
  - Publish event `mqtt_connected`.
  - Publish state hiện tại.

### Topic

| Topic | Hướng | Retain | Nội dung |
| --- | --- | --- | --- |
| `yolohome/device/yolo_uno_01/cmd` | App/backend -> device | Không | Lệnh JSON điều khiển. |
| `yolohome/device/yolo_uno_01/ack` | Device -> app/backend | Không | ACK kết quả lệnh. |
| `yolohome/device/yolo_uno_01/state` | Device -> app/backend | Có | Trạng thái actuator và kết nối. |
| `yolohome/device/yolo_uno_01/telemetry` | Device -> app/backend | Không | Dữ liệu cảm biến. |
| `yolohome/device/yolo_uno_01/event` | Device -> app/backend | Không | Sự kiện hệ thống/cửa/feedback. |
| `yolohome/device/yolo_uno_01/availability` | Device/broker -> app/backend | Có | `online` hoặc `offline`. |

### Command JSON

Base payload:

```json
{
  "commandId": "c1",
  "target": "led|fan|door|system",
  "action": "..."
}
```

`commandId` có thể là string hoặc int. Nếu thiếu, firmware tự sinh `cmd-<millis_present>`. Field `type` cũng được chấp nhận như alias của `target`.

### LED command

```json
{ "commandId": "c_led_on", "target": "led", "action": "on" }
```

```json
{ "commandId": "c_led_off", "target": "led", "action": "off" }
```

```json
{ "commandId": "c_led_auto", "target": "led", "action": "auto" }
```

```json
{ "commandId": "c_led_rgb", "target": "led", "action": "set", "r": 255, "g": 80, "b": 20 }
```

```json
{ "commandId": "c_led_hex", "target": "led", "action": "set", "hex": "#FF5014" }
```

```json
{ "commandId": "c_led_color", "target": "led", "action": "set", "color": "red" }
```

### Fan command

```json
{ "commandId": "c_fan_on", "target": "fan", "action": "on" }
```

```json
{ "commandId": "c_fan_off", "target": "fan", "action": "off" }
```

```json
{ "commandId": "c_fan_set", "target": "fan", "action": "set", "speed": 60 }
```

`value` cũng được chấp nhận thay cho `speed`.

### Door command

```json
{ "commandId": "c_door_open", "target": "door", "action": "open" }
```

```json
{ "commandId": "c_door_close", "target": "door", "action": "close" }
```

### System command

```json
{ "commandId": "c_ping", "target": "system", "action": "ping" }
```

### ACK payload

```json
{
  "schemaVersion": 1,
  "deviceId": "yolo_uno_01",
  "ts": 123456,
  "commandId": "c1",
  "source": "cmd_topic",
  "success": true,
  "message": "fan_set",
  "state": {
    "fanSpeed": 60,
    "ledEnabled": true,
    "ledMode": "manual",
    "ledR": 255,
    "ledG": 80,
    "ledB": 20,
    "doorState": 0,
    "doorStatus": "closed"
  }
}
```

### State payload

```json
{
  "schemaVersion": 1,
  "deviceId": "yolo_uno_01",
  "fwVersion": "2026.04",
  "ts": 123456,
  "mqttConnected": true,
  "wifiRssi": -58,
  "state": {
    "fanSpeed": 60,
    "ledEnabled": true,
    "ledMode": "manual",
    "ledR": 255,
    "ledG": 80,
    "ledB": 20,
    "doorState": 0,
    "doorStatus": "closed"
  }
}
```

### Telemetry payload

```json
{
  "schemaVersion": 1,
  "deviceId": "yolo_uno_01",
  "fwVersion": "2026.04",
  "ts": 123456,
  "temperature": 29.5,
  "humidity": 66.3,
  "light": 40,
  "dhtValid": true,
  "pirDetected": false
}
```

### Event payload

```json
{
  "schemaVersion": 1,
  "deviceId": "yolo_uno_01",
  "fwVersion": "2026.04",
  "ts": 123456,
  "eventType": "door",
  "message": "opened_from_cmd_topic"
}
```

## 12. Web dashboard nội bộ

Dashboard chạy bằng `WebServer` trên port 80 sau khi WiFi kết nối.

### Trang UI

- Route: `GET /`
- Hiển thị:
  - Nhiệt độ, độ ẩm, ánh sáng.
  - Trạng thái PIR.
  - Trạng thái MQTT.
  - Tốc độ quạt.
  - Màu/mode RGB.
  - Trạng thái cửa.
  - Uptime.
  - Biểu đồ 24 mẫu gần nhất cho nhiệt độ, độ ẩm, ánh sáng.
- Browser poll `/api/sensors` mỗi 2 giây.
- UI dùng Chart.js từ jsDelivr và font từ Google Fonts.

### API

| Endpoint | Method | Chức năng |
| --- | --- | --- |
| `/api/sensors` | GET | Trả JSON cảm biến, actuator, MQTT, uptime. |
| `/api/rgb?r=&g=&b=` | GET | Bật LED manual và set RGB 0..255. |
| `/api/rgb/off` | GET | Tắt LED manual/auto. |
| `/api/rgb/auto` | GET | Bật LED auto cycle. |
| `/api/fan?speed=` | GET | Set quạt 0..100%. |
| `/api/door/open` | GET | Mở cửa. |
| `/api/door/close` | GET | Đóng cửa. |
| `/api/door` | GET | Alias mở cửa. |

Tất cả API hiện trả header `Access-Control-Allow-Origin: *`.
Các API điều khiển actuator sẽ publish MQTT `state` và `ack`; ACK từ dashboard có `source = "dashboard_http"`.


