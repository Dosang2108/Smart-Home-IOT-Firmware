#include <global_var.hpp>

#ifndef WIFI_SSID
#define WIFI_SSID "Feel Coffee & Tea 3"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "camonquykhach"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER "073d03b99541400d98714c4829a277a6.s1.eu.hivemq.cloud"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 8883
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME "IotHome2026"
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD "Iot_home2026@"
#endif

// Software timers
unsigned long millis_present = 0;
unsigned long lastTime_100ms = 0;
unsigned long lastTime_1s = 0;
unsigned long lastTime_10s = 0;
unsigned long lastTime_30s = 0;

void millis_update(void)
{
    millis_present = millis();
}

// WiFi & MQTT Configuration
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_password = MQTT_PASSWORD;

// Device instances
deviceName Fan1 = {"Fan1", false, 0};
deviceName Led1 = {"Led1", false, 0};

// Sensor values
// int Value_SoilMoisture = 0;
int Value_Light = 0;
float Value_Temperature = 0.0;
float Value_Humidity = 0.0;
bool dhtDataValid = false;
bool pirRawLevel = false;
bool pirRawActive = false;
bool pirDetected = false;

// MQTT Control Variables
bool mqttLedState = false;
uint8_t mqttLedR = 255, mqttLedG = 255, mqttLedB = 255;
int mqttFanSpeed = 0;

// Password FSM
int passwordStatus = PASSWORD_STATE_CHECK;
String adminPassword = "123";
String inputPass = "";

// Door/Servo State
DoorState doorState = DOOR_CLOSED;
unsigned long doorOpenTime = 0;

// Display
bool colonVisible = true;

// FaceAI
String faceAIResult = "";

void pinSetup(void)
{
    pinMode(FAN_PIN, OUTPUT);
    // pinMode(soilMoisturePin, INPUT);
    pinMode(light, INPUT);
    pinMode(PIR_PIN, PIR_INPUT_MODE);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}
