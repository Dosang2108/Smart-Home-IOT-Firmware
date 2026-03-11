#include <Arduino.h>
#include <global_var.hpp>
#include <music_dec.hpp>
#include <MQTT.hpp>
#include <FSM_SETTING.hpp>
#include <display.hpp>
#include <sensors.hpp>
#include <control.hpp>

void setup()
{
  Serial.begin(115200);

  // Startup melody
  play_music(BUZZER_PIN);

  // Connect WiFi + MQTT
  init_Wifi_and_MQTT();

  // Initialize pins
  pinSetup();

  // Initialize NeoPixel
  NeoPixel.begin();
  NeoPixel.clear();
  NeoPixel.show();

  // LCD: init -> clear -> backlight (correct order per P2)
  lcd.init();
  lcd.clear();
  lcd.backlight();

  // NTP time sync (GMT+7)
  initNTP();

  // Servo motor
  initServo();

  // IR receiver
  initIR();

  // Register MQTT V-channels (M10)
  registerChannels();

  // Show "OK" on LCD (per P2)
  lcd.setCursor(0, 0);
  lcd.print("System OK");
  delay(1000);
  lcd.clear();

  // Initial sensor read and display
  valueSensor();
  lcdDisplayRow1();
  lcdDisplayRow2();
}

void loop()
{
  millis_update();
  mqttLoop();

  // === 100ms tasks: PIR sensor + IR remote (T6, T7) ===
  if (millis_present - lastTime_100ms >= PERIOD_100MS)
  {
    lastTime_100ms = millis_present;
    readPIR();            // T6: Check PIR every 100ms
    handleIRRemote();     // T7: Check IR remote every 100ms
    handlePIRControl();   // T8: Auto LED from PIR
    handleDoorServo();    // T9: Servo timing (open 2s, close, detach)
  }

  // === 1s task: Colon blink (T5) ===
  if (millis_present - lastTime_1s >= PERIOD_1S)
  {
    lastTime_1s = millis_present;
    colonVisible = !colonVisible;
    lcdUpdateColon();     // D3: Blinking colon on time display
  }

  // === 10s task: LCD row 1 update (T3) ===
  if (millis_present - lastTime_10s >= PERIOD_10S)
  {
    lastTime_10s = millis_present;
    lcdDisplayRow1();     // D1: Update sensor display
  }

  // === 30s tasks: Sensor read + MQTT publish + LCD row 2 (T1, T2, T4) ===
  if (millis_present - lastTime_30s >= PERIOD_30S)
  {
    lastTime_30s = millis_present;
    valueSensor();        // T1: Read sensors every 30s
    publishSensorData();  // T2: Send to MQTT (V1, V2, V3)
    lcdDisplayRow2();     // T4: Update time display
  }
}
