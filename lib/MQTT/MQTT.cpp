#include <MQTT.hpp>
#include <control.hpp>
#include <messages.hpp> 

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
  // V10: LED on/off ("1" or "0")
  if (strcmp(topic, MQTT_TOPIC_V10) == 0) {
    mqttLedState = (message[0] == '1');
    if (mqttLedState) {
      led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
      publishFeedback(MSG_LED_ON);
    } else {
      led_rgb_off();
      publishFeedback(MSG_LED_OFF);
    }
  }
  // V11: LED color code (e.g., "#FF0000")
  else if (strcmp(topic, MQTT_TOPIC_V11) == 0) {
    // Xóa khoảng trắng thừa ở đầu chuỗi nếu có
    char* cmd = message;
    while (*cmd == ' ') cmd++;

    if (strcasecmp(cmd, "auto") == 0) {
      led_rgb_set_auto(true);
      if (mqttLedState) {
        led_rgb_tick();
      }
      publishFeedback(MSG_RGB_AUTO);
      return;
    }

    led_rgb_set_auto(false);
    uint8_t r = 0, g = 0, b = 0;
    bool parsed = parseNamedColor(cmd, &r, &g, &b) ||
                  parseHexColor(message, &r, &g, &b) ||
                  parseCsvColor(message, &r, &g, &b) ||
                  parseJsonColor(message, &r, &g, &b);

    if (!parsed) {
      publishFeedback(MSG_RGB_INVALID);
      return;
    }

    mqttLedR = r;
    mqttLedG = g;
    mqttLedB = b;

    if (mqttLedState) {
      led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
    }
    publishFeedback(MSG_RGB_CHANGED);
  }
  // V12: Fan speed (string number 0-100)
  else if (strcmp(topic, MQTT_TOPIC_V12) == 0) {
    mqttFanSpeed = constrain(atoi(message), 0, 100);
    fan_set_speed(mqttFanSpeed);
    char fb[32];
    snprintf(fb, sizeof(fb), MSG_FAN_SPEED_TEMPLATE, mqttFanSpeed);
    publishFeedback(fb);
  }
  // V14: FaceAI result ("A" or "B")
  else if (strcmp(topic, MQTT_TOPIC_V14) == 0) {
    faceAIResult = String(message);
    if (message[0] == 'A') {
      servo_open();
      doorState = DOOR_OPENING;
      doorOpenTime = millis();
      publishFeedback(MSG_DOOR_OPEN_SUCCESS);
    } else if (message[0] == 'B') {
      publishFeedback(MSG_DOOR_GUEST);
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