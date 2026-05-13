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

## 13. Kiểm tra đã chạy

### Build firmware

Lệnh:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe run
```

Kết quả:

- Status: success.
- RAM: 48,488 / 327,680 bytes, khoảng 14.8%.
- Flash: 1,069,821 / 3,342,336 bytes, khoảng 32.0%.
- Không thấy lỗi compile/link trong lần build này.

### Static check

Lệnh:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe check
C:\Users\Sang\.platformio\penv\Scripts\pio.exe check --skip-packages
```

Kết quả:

- PlatformIO trả status passed.
- Cppcheck vẫn báo 1 lỗi high trong dependency `.pio/libdeps/yolo_uno/ArduinoJson/src/ArduinoJson/Polyfills/preprocessor.hpp`.
- Lỗi này nằm trong thư viện ArduinoJson dưới `.pio/libdeps`, không nằm trong source dự án. Đây nhiều khả năng là false positive/preprocessor limitation của cppcheck với macro ArduinoJson.
- Chưa có unit test trong thư mục `test`, nên chưa có test tự động cấp module.

## 14. Lỗi/rủi ro còn lại

1. Credential đang hard-code trong source.
   - `ssid`, WiFi password, MQTT server, MQTT username/password và mật khẩu cửa mặc định nằm trong `lib/global_var/global_var.cpp`.
   - Nên chuyển sang file cấu hình local không commit, NVS provisioning, captive portal, hoặc build flags/secret manager.

2. MQTT TLS đang dùng `espClient.setInsecure()`.
   - Kết nối có mã hóa nhưng không xác thực certificate broker.
   - Nên cấu hình CA certificate hoặc certificate pinning nếu triển khai thật.

3. Kết nối WiFi ở boot có thể block vô hạn.
   - `init_Wifi_and_MQTT()` chờ `WiFi.status() == WL_CONNECTED` trong vòng lặp không timeout.
   - Nếu sai WiFi hoặc mất mạng, firmware sẽ kẹt ở boot và không vào RTOS/dashboard/offline mode.

4. MQTT payload limit chưa khớp với PubSubClient.
   - Code tự đặt `MQTT_MAX_PAYLOAD_LEN = 512`.
   - PubSubClient mặc định `MQTT_MAX_PACKET_SIZE = 256` nếu không gọi `client.setBufferSize()`.
   - Payload lệnh dài có thể không vào callback như kỳ vọng.

5. Dashboard không có xác thực và dùng GET để thay đổi trạng thái.
   - Bất kỳ thiết bị nào trong LAN truy cập được IP board đều có thể bật/tắt đèn, quạt, mở cửa.
   - Header CORS `*` làm API dễ bị gọi từ trang web khác trong cùng mạng.

6. Dashboard phụ thuộc CDN ngoài.
   - Chart.js và Google Fonts được load từ Internet.
   - Nếu client không có Internet, dashboard vẫn có thể điều khiển nhưng biểu đồ/font có thể lỗi.

7. Một số chức năng khai báo nhưng chưa hoàn thiện.
   - `pump_on`, `pump_off`, `pump_control_manual` có prototype nhưng implementation đang comment.
   - `Pump1` được `extern` trong header nhưng chưa có definition.
   - Soil moisture cũng đang comment dở.
   - Build hiện vẫn pass vì các phần này chưa được dùng; nếu dùng lại sẽ dễ lỗi link/compile.

8. `messages.hpp` có một dòng `\` lẻ.
   - Build hiện vẫn pass, nhưng dòng này khó đọc và dễ gây lỗi tiền xử lý khi chỉnh sửa gần đó.

9. README hiện không còn phản ánh đúng dự án.
   - `README.md` vẫn là template PlatformIO/Espressif.
   - Code fence phần cấu hình bị thiếu đóng fence, làm Markdown render sai.
   - Nên thay bằng tài liệu ngắn trỏ tới `SPEC.md` và hướng dẫn build/upload.

10. Encoding tiếng Việt trong một số file bị lỗi mojibake.
    - Nhiều comment tiếng Việt trong source hiển thị sai encoding khi đọc bằng shell.
    - Nên chuẩn hóa toàn bộ source/documentation về UTF-8.

11. `IR_DEBUG` đang bật mặc định.
    - Serial log sẽ in raw IR code và protocol liên tục khi dùng remote.
    - Hữu ích lúc debug, nhưng nên tắt khi chạy production.

12. Trạng thái global được đọc/ghi từ nhiều task mà chưa có lock chung.
    - Các biến như `mqttLedState`, `mqttFanSpeed`, `doorState`, sensor values được dùng bởi MQTT callback, web dashboard và task control/telemetry.
    - Với kiểu nhỏ thường ít rủi ro trên ESP32, nhưng payload state có thể chụp trạng thái giữa lúc đang cập nhật.

13. Ý nghĩa lệnh LED `off` đã rõ hơn khi PIR auto light đang tắt mặc định.
    - Sau khi `led off`, PIR không tự bật đèn lại vì `PIR_AUTO_LIGHT_ENABLED = 0`.
    - Nếu cần bật/tắt PIR auto light qua MQTT/dashboard, nên thêm state riêng như `pirAutoEnabled`.

## 15. Ưu tiên xử lý đề xuất

1. Sửa bảo mật trước: bỏ hard-code credential, bật TLS verification, thêm auth cho dashboard.
2. Sửa ổn định boot: thêm WiFi timeout/offline mode và reconnect WiFi định kỳ.
3. Sửa MQTT buffer: gọi `client.setBufferSize()` đủ lớn cho payload 512 bytes hoặc giảm spec payload.
4. Dọn code/documentation: bỏ prototype chưa dùng, sửa `messages.hpp`, chuẩn hóa README và UTF-8.
5. Thêm test nhỏ cho parser lệnh MQTT và FSM mật khẩu nếu muốn duy trì dự án lâu dài.
