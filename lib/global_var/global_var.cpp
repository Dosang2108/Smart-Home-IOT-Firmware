#include <global_var.hpp>

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
const char *ssid = "ACLAB";
const char *password = "ACLAB2023";
const char *mqtt_server = "073d03b99541400d98714c4829a277a6.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char *mqtt_username = "IotHome2026";
const char *mqtt_password = "Iot_home2026@";

// Device instances
deviceName Fan1 = {"Fan1", false, 0};
deviceName Pump1 = {"Pump1", false, 0};
deviceName Led1 = {"Led1", false, 0};

// Sensor values
int Value_SoilMoisture = 0;
int Value_Light = 0;
float Value_Temperature = 0.0;
float Value_Humidity = 0.0;
bool dhtDataValid = false;
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
    pinMode(pump, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(soilMoisturePin, INPUT);
    pinMode(light, INPUT);
    pinMode(PIR_PIN, INPUT);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}