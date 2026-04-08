# MQTT Payload Specification

This document defines MQTT topics and payload formats used by this firmware.

## 1) Device Identity and Topic Namespace

- Device ID: `yolo_uno_01`
- Namespace: `yolohome/device/yolo_uno_01/`

Structured topics:

- `yolohome/device/yolo_uno_01/cmd`
- `yolohome/device/yolo_uno_01/ack`
- `yolohome/device/yolo_uno_01/state`
- `yolohome/device/yolo_uno_01/telemetry`
- `yolohome/device/yolo_uno_01/event`
- `yolohome/device/yolo_uno_01/availability`

## 2) General Notes

- Max inbound payload size handled by callback: `512` bytes.
- JSON fields commonly included:
  - `schemaVersion`
  - `deviceId`
  - `fwVersion`
  - `ts` (milliseconds from boot, based on `millis_present`)

## 3) CMD Topic

Topic:

- `yolohome/device/yolo_uno_01/cmd`

Direction:

- App/Backend -> Device

### 3.1 CMD Base JSON

```json
{
  "commandId": "c1",
  "target": "led|fan|door|system",
  "action": "..."
}
```

`commandId` can be string or int. If missing, firmware auto-generates one.

### 3.2 LED Commands

- Turn on:

```json
{ "commandId": "c_led_on", "target": "led", "action": "on" }
```

- Turn off:

```json
{ "commandId": "c_led_off", "target": "led", "action": "off" }
```

- Auto mode:

```json
{ "commandId": "c_led_auto", "target": "led", "action": "auto" }
```

- Set manual color by RGB:

```json
{ "commandId": "c_led_rgb", "target": "led", "action": "set", "r": 255, "g": 80, "b": 20 }
```

- Set manual color by HEX:

```json
{ "commandId": "c_led_hex", "target": "led", "action": "set", "hex": "#FF5014" }
```

- Set manual color by name/HEX/CSV in `color`:

```json
{ "commandId": "c_led_color", "target": "led", "action": "set", "color": "red" }
```

Supported color names: `red`, `orange`, `yellow`, `green`, `cyan`, `blue`, `purple`, `white`, `off`.

### 3.3 Fan Commands

- Turn on (100%):

```json
{ "commandId": "c_fan_on", "target": "fan", "action": "on" }
```

- Turn off (0%):

```json
{ "commandId": "c_fan_off", "target": "fan", "action": "off" }
```

- Set speed (0..100):

```json
{ "commandId": "c_fan_set", "target": "fan", "action": "set", "speed": 60 }
```

Alternative accepted key: `value`.

### 3.4 Door Commands

- Open door:

```json
{ "commandId": "c_door_open", "target": "door", "action": "open" }
```

### 3.5 System Commands

- Ping:

```json
{ "commandId": "c_ping", "target": "system", "action": "ping" }
```

## 4) ACK Topic

Topic:

- `yolohome/device/yolo_uno_01/ack`

Direction:

- Device -> App/Backend

Payload:

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
    "ledB": 20
  }
}
```

`source` can be `cmd_topic` or legacy source (`v10`, `v11`, `v12`, `v14`).

## 5) STATE Topic

Topic:

- `yolohome/device/yolo_uno_01/state`

Direction:

- Device -> App/Backend

Retain:

- `true`

Payload:

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
    "doorState": 0
  }
}
```

`doorState`: `0=DOOR_CLOSED`, `1=DOOR_OPENING`, `2=DOOR_CLOSING`.

## 6) TELEMETRY Topic

Topic:

- `yolohome/device/yolo_uno_01/telemetry`

Direction:

- Device -> App/Backend

Payload:

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

## 7) EVENT Topic

Topic:

- `yolohome/device/yolo_uno_01/event`

Direction:

- Device -> App/Backend

Payload:

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

Known examples:

- `eventType=system`, `message=mqtt_connected`
- `eventType=door`, `message=opened_from_cmd_topic`
- `eventType=face_ai`, `message=recognized_owner`
- `eventType=face_ai`, `message=recognized_guest`

## 8) AVAILABILITY Topic

Topic:

- `yolohome/device/yolo_uno_01/availability`

Direction:

- Device/Broker -> App/Backend

Payload:

- `online`
- `offline`

Retain:

- `true`

LWT behavior:

- On unexpected disconnect, broker publishes `offline` (QoS 1, retained).
- On successful reconnect, device publishes `online`.

## 9) Legacy V-Channel Payloads (Backward Compatibility)

Control topics (inbound):

- `yolohome/V10` (LED on/off):
  - Payload: `"1"` or `"0"`
- `yolohome/V11` (LED color/mode):
  - Payload accepted:
    - `"auto"`
    - named color (`red`, `green`, ...)
    - hex (`"#RRGGBB"`)
    - CSV (`"r,g,b"`)
    - JSON (`{"r":255,"g":80,"b":20}` or `{"hex":"#FF5014"}`)
- `yolohome/V12` (fan speed):
  - Payload: string number `0..100`
- `yolohome/V14` (face result):
  - Payload: `"A"` (owner), `"B"` (guest)

Status topics (outbound):

- `yolohome/V1` temperature text (example: `"29.5"`)
- `yolohome/V2` humidity text (example: `"66.3"`)
- `yolohome/V3` light text integer (example: `"40"`)
- `yolohome/V4` fan speed text integer (example: `"60"`)
- `yolohome/V5` LED color hex (example: `"#FF5014"`)
- `yolohome/V6` LED mode (`"off"`, `"manual"`, `"auto"`)
- `yolohome/V13` feedback text message

## 10) Quick End-to-End Test

1. Subscribe:

- `yolohome/device/yolo_uno_01/ack`
- `yolohome/device/yolo_uno_01/state`
- `yolohome/device/yolo_uno_01/telemetry`
- `yolohome/device/yolo_uno_01/event`
- `yolohome/device/yolo_uno_01/availability`

1. Publish command:

Topic:

- `yolohome/device/yolo_uno_01/cmd`

Payload:

```json
{ "commandId": "demo-1", "target": "fan", "action": "set", "speed": 65 }
```

1. Expected:

- ACK with `commandId=demo-1` and `success=true`
- STATE updated (`fanSpeed=65`)
- AVAILABILITY remains `online`
