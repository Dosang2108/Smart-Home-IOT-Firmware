#include <MQTT.hpp>
#include <control.hpp>

WiFiClientSecure espClient;
PubSubClient client(espClient);

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
  if (!client.connected()) reconnect();
  client.subscribe(MQTT_TOPIC_V10);
  client.subscribe(MQTT_TOPIC_V11);
  client.subscribe(MQTT_TOPIC_V12);
  client.subscribe(MQTT_TOPIC_V14);
  Serial.println("Registered MQTT channels: V10, V11, V12, V14");
}

void mqttLoop(void)
{
  if (!client.connected()) reconnect();
  client.loop();
}

void publishSensorData(void)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1f", Value_Temperature);
  client.publish(MQTT_TOPIC_V1, buf);

  snprintf(buf, sizeof(buf), "%.1f", Value_Humidity);
  client.publish(MQTT_TOPIC_V2, buf);

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

// Parse hex color string like "#FF00AA" or "FF00AA"
static void parseColorCode(const char* colorStr)
{
  const char* hex = colorStr;
  if (hex[0] == '#') hex++;
  if (strlen(hex) >= 6) {
    char rStr[3] = {hex[0], hex[1], '\0'};
    char gStr[3] = {hex[2], hex[3], '\0'};
    char bStr[3] = {hex[4], hex[5], '\0'};
    mqttLedR = (uint8_t)strtol(rStr, NULL, 16);
    mqttLedG = (uint8_t)strtol(gStr, NULL, 16);
    mqttLedB = (uint8_t)strtol(bStr, NULL, 16);
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  if (payload == nullptr || length == 0) return;

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
    parseColorCode(message);
    if (mqttLedState) {
      led_rgb_set(mqttLedR, mqttLedG, mqttLedB);
    }
    publishFeedback("Da doi mau den");
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
  while (!client.connected())
  {
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
      Serial.printf("Failed, rc=%d. Retrying in 5s...\n", client.state());
      delay(5000);
    }
  }
}


