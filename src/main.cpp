#include <Arduino.h>
#include <global_var.hpp>
#include <music_dec.hpp>
#include <MQTT.hpp>
#include <FSM_SETTING.hpp>
#include <display.hpp>
#include <sensors.hpp>

void setup()
{
  Serial.begin(115200);
  play_music(BUZZER_PIN); // Chơi nhạc khi khởi động
  init_Wifi_and_MQTT();
  pinSetup();
  NeoPixel.begin();
  lcd.clear();
  lcd.init();
  lcd.backlight();

}

void loop()
{
  FSM_Main();
  valueSensor();
  millis_update();
  Transmit_receiver_data();
  // put your main code here, to run repeatedly:
  lcdDisplay();
}
