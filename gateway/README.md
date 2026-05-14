# YoloHome Store-and-Forward Gateway

This Python gateway bridges a local MQTT broker to HiveMQ Cloud and keeps a durable SQLite queue while the cloud connection is unavailable.

## Flow

1. ESP32-S3 publishes `telemetry`, `state`, `event`, `ack`, and `availability` to a local MQTT broker, for example Mosquitto on a Raspberry Pi or mini PC.
2. The gateway subscribes to those local topics.
3. If HiveMQ Cloud is online, the gateway forwards the message immediately.
4. If HiveMQ Cloud is offline, the gateway stores safe uplink messages in SQLite.
5. When HiveMQ Cloud reconnects, the gateway publishes queued records in `id` order and deletes each record only after publish ACK.

`availability` is forwarded only live and is not replayed after an outage. `cmd` is also not stored by default, because delayed actuator commands can be unsafe.

## Install

```powershell
cd gateway
python -m venv .venv
.\.venv\Scripts\pip install -r requirements.txt
Copy-Item config.example.json config.json
```

Edit `config.json` with your local broker and HiveMQ Cloud credentials.

## ESP32 Firmware Mode

The default `yolo_uno` PlatformIO environment still connects directly to HiveMQ Cloud with TLS.

Use `yolo_uno_gateway` when the ESP32 should publish to a plain local MQTT broker:

```powershell
C:\Users\Sang\.platformio\penv\Scripts\pio.exe run -e yolo_uno_gateway
```

Before flashing that environment, edit `MQTT_SERVER` in `platformio.ini` to the IP address of the local broker/Gateway machine.

## Run

```powershell
.\.venv\Scripts\python store_forward_gateway.py --config config.json
```

## Quick Local Test

Start Mosquitto locally, run the gateway, then publish a fake telemetry message:

```powershell
mosquitto_pub -h 127.0.0.1 -t yolohome/device/yolo_uno_01/telemetry -m "{\"schemaVersion\":1,\"deviceId\":\"yolo_uno_01\",\"ts\":1,\"temperature\":29.5}"
```

If the cloud config is offline or wrong, the message is stored:

```powershell
sqlite3 gateway_buffer.db "select id, topic, created_at from outbound_queue;"
```

After fixing cloud connectivity, the queue should drain:

```powershell
sqlite3 gateway_buffer.db "select count(*) from outbound_queue;"
```
