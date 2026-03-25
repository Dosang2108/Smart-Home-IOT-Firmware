#ifndef GLOBAL_VAR_HPP
#define GLOBAL_VAR_HPP

#include <Arduino.h>
#include "Wire.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ============ GPIO Configuration ============

// #define soilMoisturePin 1    // A0 (GPIO1)
#define light 2              // A1 (GPIO2)
#define FAN_PIN 10           // Fan pin (GPIO10)
#define PIN_NEO_PIXEL 18      // RGB NeoPixel pin (GPIO6)
#define NUM_PIXELS 4
#define PIR_PIN GPIO_NUM_7          // PIR motion sensor (adjust to your board)
#define IR_RECV_PIN GPIO_NUM_9       // IR receiver pin (adjust to your board)
#define SERVO_PIN 4          // Servo motor pin (adjust to your board)
#define I2C_SCL_PIN GPIO_NUM_12       // I2C SCL (GPIO12)
#define I2C_SDA_PIN GPIO_NUM_11       // I2C SDA (GPIO11)
#define LCD_COLUMNS 16
#define LCD_ROWS 2

// ============ ID Setup ============
#define ID_area_send 2
#define ID_area_recv 2

// ============ MQTT V-Channel Topics ============
#define MQTT_TOPIC_V1   "yolohome/V1"
#define MQTT_TOPIC_V2   "yolohome/V2"
#define MQTT_TOPIC_V3   "yolohome/V3"
#define MQTT_TOPIC_V10  "yolohome/V10"
#define MQTT_TOPIC_V11  "yolohome/V11"
#define MQTT_TOPIC_V12  "yolohome/V12"
#define MQTT_TOPIC_V13  "yolohome/V13"
#define MQTT_TOPIC_V14  "yolohome/V14"

// ============ Timer Periods (ms) ============
#define PERIOD_100MS  100
#define PERIOD_1S     1000
#define PERIOD_10S    10000
#define PERIOD_30S    30000

// ============ Software Timer ============
    void millis_update(void);
    extern unsigned long millis_present;
    extern unsigned long lastTime_100ms;
    extern unsigned long lastTime_1s;
    extern unsigned long lastTime_10s;
    extern unsigned long lastTime_30s;

// ============ Device Struct ============
    struct deviceName
    {
        String name;
        bool active;
        int value;
    };
    extern deviceName Fan1;
    extern deviceName Pump1;
    extern deviceName Led1;

// ============ Sensor Variables ============
    // extern int Value_SoilMoisture;
    extern int Value_Light;
    extern float Value_Temperature;
    extern float Value_Humidity;
    extern bool dhtDataValid;
    extern bool pirDetected;

// ============ WiFi & MQTT Config ============
    extern const char *ssid;
    extern const char *password;
    extern const char *mqtt_server;
    extern const int mqtt_port;
    extern const char *mqtt_username;
    extern const char *mqtt_password;

// ============ MQTT Control Variables ============
    extern bool mqttLedState;
    extern uint8_t mqttLedR, mqttLedG, mqttLedB;
    extern int mqttFanSpeed;

// ============ Password FSM ============
#define PASSWORD_STATE_CHECK  0
#define PASSWORD_STATE_CHANGE 1
#define DOOR_OPEN_DURATION    2000
    extern int passwordStatus;
    extern String adminPassword;
    extern String inputPass;

    enum DoorState { DOOR_CLOSED, DOOR_OPENING, DOOR_CLOSING };
    extern DoorState doorState;
    extern unsigned long doorOpenTime;

// ============ Display ============
    extern bool colonVisible;

// ============ FaceAI ============
    extern String faceAIResult;

// ============ Pin Setup ============
    void pinSetup(void);

#ifdef __cplusplus
}
#endif

#endif