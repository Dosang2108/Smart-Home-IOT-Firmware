#include <MQTT.hpp>
#include <control.hpp>

WiFiClientSecure espClient;
PubSubClient client(espClient);
static unsigned long lastReconnectAttemptMs = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 5000;

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

static bool parseJsonColor(const char* payload, uint8_t* r, uint8_t* g, uint8_t* b)
{
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) return false;

  if (doc["r"].is<int>() && doc["g"].is<int>() && doc["b"].is<int>()) {
    *r = (uint8_t)constrain(doc["r"].as<int>(), 0, 255);
    *g = (uint8_t)constrain(doc["g"].as<int>(), 0, 255);
    *b = (uint8_t)constrain(doc["b"].as<int>(), 0, 255);
    return true;
  }
  if (doc["hex"].is<const char*>()) {
    return parseHexColor(doc["hex"].as<const char*>(), r, g, b);
  }
  return false;
}

static bool parseNamedColor(const String& colorName, uint8_t* r, uint8_t* g, uint8_t* b)
{
  if (colorName == "red")       { *r = 255; *g = 0;   *b = 0;   return true; }
  if (colorName == "orange")    { *r = 255; *g = 127; *b = 0;   return true; }
  if (colorName == "yellow")    { *r = 255; *g = 255; *b = 0;   return true; }
  if (colorName == "green")     { *r = 0;   *g = 255; *b = 0;   return true; }
  if (colorName == "cyan")      { *r = 0;   *g = 255; *b = 255; return true; }
  if (colorName == "blue")      { *r = 0;   *g = 0;   *b = 255; return true; }
  if (colorName == "purple")    { *r = 148; *g = 0;   *b = 211; return true; }
  if (colorName == "white")     { *r = 255; *g = 255; *b = 255; return true; }
  if (colorName == "off")       { *r = 0;   *g = 0;   *b = 0;   return true; }
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

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);
}

void registerChannels(void)
{
  reconnect();
  if (client.connected()) {
    client.subscribe(MQTT_TOPIC_V10);
    client.subscribe(MQTT_TOPIC_V11);
    client.subscribe(MQTT_TOPIC_V12);
    client.subscribe(MQTT_TOPIC_V14);
    Serial.println("Registered MQTT channels: V10, V11, V12, V14");
  }
}

void mqttLoop(void)
{
  if (!client.connected()) {
    reconnect();
    return;
  }
  client.loop();
}

void publishSensorData(void)
{
  char buf[16];
  if (dhtDataValid) {
    snprintf(buf, sizeof(buf), "%.1f", Value_Temperature);
    client.publish(MQTT_TOPIC_V1, buf);

    snprintf(buf, sizeof(buf), "%.1f", Value_Humidity);
    client.publish(MQTT_TOPIC_V2, buf);
  } else {
    Serial.println("Skip publish V1/V2: DHT20 data invalid");
  }

  snprintf(buf, sizeof(buf), "%d", Value_Light);
  client.publish(MQTT_TOPIC_V3, buf);

  Serial.printf("Published: V1=%.1f V2=%.1f V3=%d\n",
                Value_Temperature, Value_Humidity, Value_Light);
}

void publishFeedback(const char* message)
{
  client.publish(MQTT_TOPIC_V13, message);
  Serial.printf("Feedback V13: %s\n", message);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  if (payload == nullptr || length == 0) return;
  if (length > 256) {
    Serial.println("Lỗi: Payload MQTT quá lớn, bo qua de bao ve he thong!");
    return;
  }

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.printf("MQTT [%s]: %s\n", topic, message);

  String topicStr(topic);

  // V10: LED on/off ("1" or "0")
  if (topicStr == MQTT_TOPIC_V10) {
    mqttLedState = (message[0] == '1');
    if (mqttLedState) {
      led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
      publishFeedback("Den da bat");
    } else {
      led_rgb_off();
      publishFeedback("Den da tat");
    }
  }
  // V11: LED color code (e.g., "#FF0000")
  else if (topicStr == MQTT_TOPIC_V11) {
    String command = String(message);
    command.trim();
    command.toLowerCase();

    if (command == "auto") {
      led_rgb_set_auto(true);
      if (mqttLedState) {
        led_rgb_tick();
      }
      publishFeedback("RGB auto cycle");
      return;
    }

    led_rgb_set_auto(false);

    uint8_t r = 0, g = 0, b = 0;
    bool parsed = parseNamedColor(command, &r, &g, &b) ||
                  parseHexColor(message, &r, &g, &b) ||
                  parseCsvColor(message, &r, &g, &b) ||
                  parseJsonColor(message, &r, &g, &b);

    if (!parsed) {
      publishFeedback("Sai dinh dang mau RGB");
      return;
    }

    mqttLedR = r;
    mqttLedG = g;
    mqttLedB = b;

    if (mqttLedState) {
      led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
    }
    publishFeedback("Da doi mau den RGB");
  }
  // V12: Fan speed (string number 0-100)
  else if (topicStr == MQTT_TOPIC_V12) {
    mqttFanSpeed = constrain(atoi(message), 0, 100);
    fan_set_speed(mqttFanSpeed);
    char fb[32];
    snprintf(fb, sizeof(fb), "Quat: %d%%", mqttFanSpeed);
    publishFeedback(fb);
  }
  // V14: FaceAI result ("A" or "B")
  else if (topicStr == MQTT_TOPIC_V14) {
    faceAIResult = String(message);
    if (faceAIResult == "A") {
      servo_open();
      doorState = DOOR_OPENING;
      doorOpenTime = millis();
      publishFeedback("Nhan dien thanh cong - Mo cua");
    } else if (faceAIResult == "B") {
      publishFeedback("Nhan dien: Khach");
    }
  }
}

void reconnect()
{
  if (client.connected()) {
    return;
  }

  unsigned long now = millis();
  if (lastReconnectAttemptMs != 0 && (now - lastReconnectAttemptMs) < RECONNECT_INTERVAL_MS) {
    return;
  }
  lastReconnectAttemptMs = now;

  Serial.print("Connecting MQTT...");
  String clientId = "ESP32Client-" + String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
  {
    Serial.println("MQTT connected");
    client.subscribe(MQTT_TOPIC_V10);
    client.subscribe(MQTT_TOPIC_V11);
    client.subscribe(MQTT_TOPIC_V12);
    client.subscribe(MQTT_TOPIC_V14);
  }
  else
  {
    Serial.printf("Failed, rc=%d. Next retry in 5s\n", client.state());
  }
}

bool isMqttConnected(void)
{
  return client.connected();
}