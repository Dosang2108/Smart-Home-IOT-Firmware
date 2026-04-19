#include <MQTT.hpp>
#include <control.hpp>
#include <messages.hpp>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

WiFiClientSecure espClient;
PubSubClient client(espClient);
static unsigned long lastReconnectAttemptMs = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 5000;
static const unsigned int MQTT_MAX_PAYLOAD_LEN = 512;
static SemaphoreHandle_t mqttClientMutex = nullptr;

static bool mqttClientLock(TickType_t timeoutTicks = pdMS_TO_TICKS(1000))
{
  if (mqttClientMutex == nullptr) {
    return false;
  }
  return xSemaphoreTakeRecursive(mqttClientMutex, timeoutTicks) == pdTRUE;
}

static void mqttClientUnlock(void)
{
  if (mqttClientMutex != nullptr) {
    xSemaphoreGiveRecursive(mqttClientMutex);
  }
}

static bool mqttPublish(const char* topic, const char* payload, bool retained = false)
{
  if (!mqttClientLock()) {
    return false;
  }
  bool ok = client.publish(topic, payload, retained);
  mqttClientUnlock();
  return ok;
}

static bool mqttSubscribe(const char* topic)
{
  if (!mqttClientLock()) {
    return false;
  }
  bool ok = client.subscribe(topic);
  mqttClientUnlock();
  return ok;
}

static bool parseHexColor(const char* colorStr, uint8_t* r, uint8_t* g, uint8_t* b)
{
  const char* hex = colorStr;
  while (*hex == ' ') hex++;
  if (hex[0] == '#') hex++;
  if (strlen(hex) < 6) return false;

  char rStr[3] = {hex[0], hex[1], '\0'};
  char gStr[3] = {hex[2], hex[3], '\0'};
  char bStr[3] = {hex[4], hex[5], '\0'};
  *r = (uint8_t)strtol(rStr, NULL, 16);
  *g = (uint8_t)strtol(gStr, NULL, 16);
  *b = (uint8_t)strtol(bStr, NULL, 16);
  return true;
}

static bool parseCsvColor(const char* colorStr, uint8_t* r, uint8_t* g, uint8_t* b)
{
  int rr, gg, bb;
  if (sscanf(colorStr, "%d,%d,%d", &rr, &gg, &bb) != 3) {
    return false;
  }
  *r = (uint8_t)constrain(rr, 0, 255);
  *g = (uint8_t)constrain(gg, 0, 255);
  *b = (uint8_t)constrain(bb, 0, 255);
  return true;
}

static bool parseNamedColor(const char* colorName, uint8_t* r, uint8_t* g, uint8_t* b)
{
  if (strcasecmp(colorName, "red") == 0)    { *r = 255; *g = 0;   *b = 0;   return true; }
  if (strcasecmp(colorName, "orange") == 0) { *r = 255; *g = 127; *b = 0;   return true; }
  if (strcasecmp(colorName, "yellow") == 0) { *r = 255; *g = 255; *b = 0;   return true; }
  if (strcasecmp(colorName, "green") == 0)  { *r = 0;   *g = 255; *b = 0;   return true; }
  if (strcasecmp(colorName, "cyan") == 0)   { *r = 0;   *g = 255; *b = 255; return true; }
  if (strcasecmp(colorName, "blue") == 0)   { *r = 0;   *g = 0;   *b = 255; return true; }
  if (strcasecmp(colorName, "purple") == 0) { *r = 148; *g = 0;   *b = 211; return true; }
  if (strcasecmp(colorName, "white") == 0)  { *r = 255; *g = 255; *b = 255; return true; }
  if (strcasecmp(colorName, "off") == 0)    { *r = 0;   *g = 0;   *b = 0;   return true; }
  return false;
}

static const char* getLedMode(void)
{
  if (!mqttLedState) {
    return "off";
  }
  return led_rgb_is_auto() ? "auto" : "manual";
}

static void publishAvailability(const char* status)
{
  mqttPublish(MQTT_TOPIC_AVAILABILITY, status, true);
}

static void publishEvent(const char* eventType, const char* message)
{
  JsonDocument doc;
  doc["schemaVersion"] = MQTT_SCHEMA_VERSION;
  doc["deviceId"] = MQTT_DEVICE_ID;
  doc["fwVersion"] = MQTT_FIRMWARE_VERSION;
  doc["ts"] = millis_present;
  doc["eventType"] = eventType;
  doc["message"] = message;

  String payload;
  serializeJson(doc, payload);
  mqttPublish(MQTT_TOPIC_EVENT, payload.c_str());
}

static void publishStateJson(void)
{
  JsonDocument doc;
  doc["schemaVersion"] = MQTT_SCHEMA_VERSION;
  doc["deviceId"] = MQTT_DEVICE_ID;
  doc["fwVersion"] = MQTT_FIRMWARE_VERSION;
  doc["ts"] = millis_present;
  doc["mqttConnected"] = isMqttConnected();
  doc["wifiRssi"] = WiFi.RSSI();

  JsonObject state = doc["state"].to<JsonObject>();
  state["fanSpeed"] = mqttFanSpeed;
  state["ledEnabled"] = mqttLedState;
  state["ledMode"] = getLedMode();
  state["ledR"] = mqttLedState ? mqttLedR : 0;
  state["ledG"] = mqttLedState ? mqttLedG : 0;
  state["ledB"] = mqttLedState ? mqttLedB : 0;
  state["doorState"] = (int)doorState;
  state["doorStatus"] = door_state_to_text(doorState);

  String payload;
  serializeJson(doc, payload);
  mqttPublish(MQTT_TOPIC_STATE, payload.c_str(), true);
}

static void publishTelemetryJson(void)
{
  JsonDocument doc;
  doc["schemaVersion"] = MQTT_SCHEMA_VERSION;
  doc["deviceId"] = MQTT_DEVICE_ID;
  doc["fwVersion"] = MQTT_FIRMWARE_VERSION;
  doc["ts"] = millis_present;
  doc["temperature"] = Value_Temperature;
  doc["humidity"] = Value_Humidity;
  doc["light"] = Value_Light;
  doc["dhtValid"] = dhtDataValid;
  doc["pirDetected"] = pirDetected;

  String payload;
  serializeJson(doc, payload);
  mqttPublish(MQTT_TOPIC_TELEMETRY, payload.c_str());
}

static void publishCommandAck(const String& commandId, bool success, const char* message, const char* source)
{
  JsonDocument doc;
  doc["schemaVersion"] = MQTT_SCHEMA_VERSION;
  doc["deviceId"] = MQTT_DEVICE_ID;
  doc["ts"] = millis_present;
  doc["commandId"] = commandId.length() ? commandId : "none";
  doc["source"] = source;
  doc["success"] = success;
  doc["message"] = message;

  JsonObject state = doc["state"].to<JsonObject>();
  state["fanSpeed"] = mqttFanSpeed;
  state["ledEnabled"] = mqttLedState;
  state["ledMode"] = getLedMode();
  state["ledR"] = mqttLedState ? mqttLedR : 0;
  state["ledG"] = mqttLedState ? mqttLedG : 0;
  state["ledB"] = mqttLedState ? mqttLedB : 0;
  state["doorState"] = (int)doorState;
  state["doorStatus"] = door_state_to_text(doorState);

  String payload;
  serializeJson(doc, payload);
  mqttPublish(MQTT_TOPIC_ACK, payload.c_str());
}

void publishActuatorStatus(void)
{
  publishStateJson();
  Serial.println("Published structured state payload");
}

static bool parseCommandId(JsonDocument& doc, String* commandId)
{
  if (doc["commandId"].is<const char*>()) {
    *commandId = String(doc["commandId"].as<const char*>());
    return true;
  }
  if (doc["commandId"].is<int>()) {
    *commandId = String(doc["commandId"].as<int>());
    return true;
  }
  *commandId = String("cmd-") + String(millis_present);
  return false;
}

static bool parseRgbCommand(JsonDocument& doc, uint8_t* r, uint8_t* g, uint8_t* b)
{
  if (doc["r"].is<int>() && doc["g"].is<int>() && doc["b"].is<int>()) {
    *r = (uint8_t)constrain(doc["r"].as<int>(), 0, 255);
    *g = (uint8_t)constrain(doc["g"].as<int>(), 0, 255);
    *b = (uint8_t)constrain(doc["b"].as<int>(), 0, 255);
    return true;
  }
  if (doc["hex"].is<const char*>()) {
    return parseHexColor(doc["hex"].as<const char*>(), r, g, b);
  }
  if (doc["color"].is<const char*>()) {
    const char* color = doc["color"].as<const char*>();
    return parseNamedColor(color, r, g, b) || parseHexColor(color, r, g, b) || parseCsvColor(color, r, g, b);
  }
  return false;
}

static bool handleStructuredCommand(const char* payload, String* commandId, const char** detail)
{
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    *detail = "invalid_json";
    return false;
  }

  parseCommandId(doc, commandId);

  const char* target = "";
  if (doc["target"].is<const char*>()) {
    target = doc["target"].as<const char*>();
  } else if (doc["type"].is<const char*>()) {
    target = doc["type"].as<const char*>();
  }

  const char* action = "";
  if (doc["action"].is<const char*>()) {
    action = doc["action"].as<const char*>();
  }

  if (strcasecmp(target, "led") == 0) {
    if (strcasecmp(action, "on") == 0) {
      mqttLedState = true;
      if (led_rgb_is_auto()) {
        led_rgb_tick();
      } else {
        led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
      }
      *detail = "led_on";
      return true;
    }
    if (strcasecmp(action, "off") == 0) {
      mqttLedState = false;
      led_rgb_set_auto(false);
      led_rgb_off();
      *detail = "led_off";
      return true;
    }
    if (strcasecmp(action, "auto") == 0) {
      mqttLedState = true;
      led_rgb_set_auto(true);
      led_rgb_tick();
      *detail = "led_auto";
      return true;
    }
    if (strcasecmp(action, "set") == 0) {
      uint8_t r = 0, g = 0, b = 0;
      if (!parseRgbCommand(doc, &r, &g, &b)) {
        *detail = "invalid_led_color";
        return false;
      }
      mqttLedState = true;
      mqttLedR = r;
      mqttLedG = g;
      mqttLedB = b;
      led_rgb_set_auto(false);
      led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
      *detail = "led_set";
      return true;
    }
    *detail = "invalid_led_action";
    return false;
  }

  if (strcasecmp(target, "fan") == 0) {
    if (strcasecmp(action, "on") == 0) {
      mqttFanSpeed = 100;
      fan_set_speed(mqttFanSpeed);
      *detail = "fan_on";
      return true;
    }
    if (strcasecmp(action, "off") == 0) {
      mqttFanSpeed = 0;
      fan_set_speed(mqttFanSpeed);
      *detail = "fan_off";
      return true;
    }
    if (strcasecmp(action, "set") == 0) {
      int speed = -1;
      if (doc["speed"].is<int>()) speed = doc["speed"].as<int>();
      else if (doc["value"].is<int>()) speed = doc["value"].as<int>();

      if (speed < 0) {
        *detail = "missing_fan_speed";
        return false;
      }
      mqttFanSpeed = constrain(speed, 0, 100);
      fan_set_speed(mqttFanSpeed);
      *detail = "fan_set";
      return true;
    }
    *detail = "invalid_fan_action";
    return false;
  }

  if (strcasecmp(target, "door") == 0) {
    if (strcasecmp(action, "open") == 0) {
      door_command_open();
      publishEvent("door", "opened_from_cmd_topic");
      *detail = "door_open";
      return true;
    }
    if (strcasecmp(action, "close") == 0) {
      door_command_close();
      publishEvent("door", "closed_from_cmd_topic");
      *detail = "door_close";
      return true;
    }
    *detail = "invalid_door_action";
    return false;
  }

  if (strcasecmp(target, "system") == 0 && strcasecmp(action, "ping") == 0) {
    *detail = "pong";
    return true;
  }

  *detail = "invalid_target";
  return false;
}

void init_Wifi_and_MQTT(void)
{
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  if (mqttClientMutex == nullptr) {
    mqttClientMutex = xSemaphoreCreateRecursiveMutex();
    if (mqttClientMutex == nullptr) {
      Serial.println("Error: failed to create MQTT mutex");
    }
  }
}

void registerChannels(void)
{
  reconnect();
  if (isMqttConnected()) {
    mqttSubscribe(MQTT_TOPIC_CMD);
    Serial.println("Registered MQTT channels: cmd");
  }
}

void mqttLoop(void)
{
  if (!isMqttConnected()) {
    reconnect();
    return;
  }
  if (mqttClientLock(pdMS_TO_TICKS(100))) {
    client.loop();
    mqttClientUnlock();
  }
}

void publishSensorData(void)
{
  publishTelemetryJson();
  publishActuatorStatus();

  Serial.printf("Published telemetry: T=%.1f H=%.1f L=%d DHT=%d\n",
                Value_Temperature, Value_Humidity, Value_Light, dhtDataValid ? 1 : 0);
}

void publishFeedback(const char* message)
{
  publishEvent("feedback", message);
  Serial.printf("Feedback event: %s\n", message);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  if (payload == nullptr || length == 0) return;
  Serial.print("MQTT topic: ");
  Serial.println(topic);
  Serial.print("Payload text: ");
for (unsigned int i = 0; i < length; i++) {
Serial.print((char)payload[i]);
}
Serial.println();

Serial.print("Payload hex : ");
for (unsigned int i = 0; i < length; i++) {
if (payload[i] < 16) Serial.print('0');
Serial.print(payload[i], HEX);
Serial.print(' ');
}
Serial.println();
Serial.println("---------------------------");
  if (length > MQTT_MAX_PAYLOAD_LEN) {
    Serial.println("Error: MQTT payload too large, message ignored");
    return;
  }

  char message[MQTT_MAX_PAYLOAD_LEN + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.printf("MQTT [%s]: %s\n", topic, message);

  if (strcmp(topic, MQTT_TOPIC_CMD) == 0) {
    String commandId;
    const char* detail = "unknown";
    bool ok = handleStructuredCommand(message, &commandId, &detail);

    if (ok) {
      publishActuatorStatus();
      publishCommandAck(commandId, true, detail, "cmd_topic");
      if (strcmp(detail, "door_open") == 0) {
        publishFeedback(MSG_DOOR_OPEN_SUCCESS);
      } else if (strcmp(detail, "door_close") == 0) {
        publishFeedback(MSG_DOOR_CLOSE_SUCCESS);
      }
    } else {
      publishCommandAck(commandId, false, detail, "cmd_topic");
    }
    return;
  }

  Serial.println("MQTT topic ignored (not subscribed in structured mode)");
}

void reconnect()
{
  if (!mqttClientLock(pdMS_TO_TICKS(2000))) {
    return;
  }

  if (client.connected()) {
    mqttClientUnlock();
    return;
  }

  unsigned long now = millis();
  if (lastReconnectAttemptMs != 0 && (now - lastReconnectAttemptMs) < RECONNECT_INTERVAL_MS) {
    mqttClientUnlock();
    return;
  }
  lastReconnectAttemptMs = now;

  Serial.print("Connecting MQTT...");
  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  if (client.connect(
      clientId.c_str(),
      mqtt_username,
      mqtt_password,
      MQTT_TOPIC_AVAILABILITY,
      1,
      true,
      "offline"))
  {
    Serial.println("MQTT connected");
    mqttSubscribe(MQTT_TOPIC_CMD);

    publishAvailability("online");
    publishEvent("system", "mqtt_connected");
    publishActuatorStatus();
    mqttClientUnlock();
  }
  else
  {
    Serial.printf("Failed, rc=%d. Next retry in 5s\n", client.state());
    mqttClientUnlock();
  }
}

bool isMqttConnected(void)
{
  if (!mqttClientLock(pdMS_TO_TICKS(100))) {
    return false;
  }
  bool connected = client.connected();
  mqttClientUnlock();
  return connected;
}
