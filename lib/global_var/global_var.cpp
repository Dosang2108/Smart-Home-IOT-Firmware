#include <global_var.hpp>

// software timer configuration
unsigned long millis_present = 0;
void millis_update(void)
{
    millis_present = millis();
}

/// Configuration MQTT Server and Wifi Connection
const char *ssid = "ACLAB";
const char *password = "ACLAB2023";

const char *mqtt_server = "073d03b99541400d98714c4829a277a6.s1.eu.hivemq.cloud";
const int   mqtt_port = 8883;
const char *mqtt_username = "IotHome2026";
const char *mqtt_password = "Iot_home2026@";

const char *topicSub = "website/sensors/esp32"; // Topic nhận dữ liệu
const char *topicPub = "iot/sensors/esp32";     // Topic gửi dữ liệu

deviceName Fan1 = {"Fan1", false, 0};
deviceName Pump1 = {"Pump1", false, 0};
deviceName Led1 = {"Led1", false, 0};

deviceName Fan2 = {"Fan2", false, 0};
deviceName Pump2 = {"Pump2", false, 0};
deviceName Led2 = {"Led2", false, 0};

// biến từ cảm biến
int Value_SoilMoisture = 0;
int Value_Light = 0;
float Value_Temperature = 0.0;
float Value_Humidity = 0.0;
bool State_FSM = true;

void pinSetup(void)
{
    pinMode(pump, OUTPUT);
    pinMode(fan, OUTPUT);
    pinMode(soilMoisturePin, INPUT);
    Wire1.begin(21, 22); // chân của I2C
}