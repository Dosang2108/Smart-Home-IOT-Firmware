#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include "global_var.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    extern Adafruit_NeoPixel NeoPixel;

    // Pump
    void pump_on();
    void pump_off();
    void pump_control_manual(int pumpspeed);

    // Fan
    void fan_set_speed(int speed);
    void fan_control_manual(bool active);

    // LED RGB
    void led_rgb_set(uint8_t r, uint8_t g, uint8_t b);
    void led_rgb_off();
    void ledwhite_on();
    void ledwhite_off();
    void ledred_on();
    void ledgreen_on();
    void led_off();

    // PIR auto light control
    void handlePIRControl();

    // Servo
    void initServo();
    void servo_open();
    void servo_close();
    void servo_detach_motor();
    void handleDoorServo();

#ifdef __cplusplus
}
#endif
#endif