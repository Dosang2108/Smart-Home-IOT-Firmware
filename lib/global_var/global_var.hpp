#ifndef GLOBAL_VAR_HPP
#define GLOBAL_VAR_HPP

#include <Arduino.h>
#include "Wire.h"
// #include "control.hpp"
// #include "display.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

// GPIO configuration
#define BUZZER_PIN 25      //  BUZZER_PIN
#define soilMoisturePin 32 // cảm biến đất: P0
#define light 33           // cảm biến ánh sáng: P1
#define pump 19            // P14
#define fan 27             // P2
#define PIN_NEO_PIXEL 26   // P10
#define NUM_PIXELS 4

// ID setup
#define ID_area_send 2
#define ID_area_recv 2

    // software timer configuration
    void millis_update(void);
    extern unsigned long millis_present;
#define priod_receiver_data 1000

    // variable receiver data
    struct deviceName
    {
        String name;
        bool active;
        int value;
    };

    // variable receiver data
    extern deviceName Fan1;
    extern deviceName Pump1;
    extern deviceName Led1;

    extern deviceName Fan2;
    extern deviceName Pump2;
    extern deviceName Led2;

    extern bool State_FSM;
    /// Configuration MQTT Server and Wifi Connection
    extern const char *ssid;
    extern const char *password;
    extern const char *mqtt_server;
    extern const int  mqtt_port;
    extern const char *mqtt_username;
    extern const char *mqtt_password;
    // const char* topicSub = "0rm10p/read/Mario@M7";  // Topic nhận dữ liệu
    extern const char *topicSub; // Topic nhận dữ liệu
    extern const char *topicPub; // Topic gửi dữ liệu

    // biến từ cảm biến
    extern int Value_SoilMoisture;
    extern int Value_Light;
    extern float Value_Temperature;
    extern float Value_Humidity;

    void pinSetup(void);

#ifdef __cplusplus
}
#endif

#endif