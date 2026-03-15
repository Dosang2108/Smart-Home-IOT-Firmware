#include <Arduino.h>
#include <global_var.hpp>
#include <music_dec.hpp>
#include <MQTT.hpp>
#include <FSM_SETTING.hpp>
#include <display.hpp>
#include <sensors.hpp>
#include <sensor_dashboard.hpp>
#include <control.hpp>

void setup()
{
  Serial.begin(115200);

  // Startup melody
  play_music(BUZZER_PIN);

  // Initialize pins
  pinSetup();

  // Scan I2C bus to detect LCD address (check Serial Monitor)
  scanI2CBus();

  // Initialize NeoPixel
  NeoPixel.begin();
  NeoPixel.clear();
  NeoPixel.show();

  // LCD init style aligned with reference code
  initLCD();

  // Connect WiFi + MQTT
  init_Wifi_and_MQTT();

  lcd.setCursor(0, 1);
  lcd.print("WiFi connected ");

  // NTP time sync (GMT+7)
  initNTP();

  // DHT20 sensor init
  initDHT20();

  // Servo motor
  initServo();

  // IR receiver
  initIR();

  // Register MQTT V-channels (M10)
  registerChannels();

  // Start local web dashboard for sensors and basic controls
  initSensorDashboard();

  if (isMqttConnected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT connected");
    lcd.setCursor(0, 1);
    lcd.print("System ready");
    delay(800);
  }

  // Show "OK" on LCD (per P2)
  lcd.setCursor(0, 0);
  lcd.print("System OK");
  delay(1000);
  lcd.clear();

  // Initial sensor read and display
  valueSensor();
  lcdDisplaySensors();
}

void loop()
{
  millis_update();
  mqttLoop();
  handleSensorDashboard();
  lcdDisplaySensors();

  // === 100ms tasks: PIR sensor + IR remote (T6, T7) ===
  if (millis_present - lastTime_100ms >= PERIOD_100MS)
  {
    lastTime_100ms = millis_present;
    led_rgb_tick();       // RGB auto cycle (if enabled)
    readPIR();            // T6: Check PIR every 100ms
    handleIRRemote();     // T7: Check IR remote every 100ms
    handlePIRControl();   // T8: Auto LED from PIR
    handleDoorServo();    // T9: Servo timing (open 2s, close, detach)
  }

  // === 30s tasks: Sensor read + MQTT publish (T1, T2) ===
  if (millis_present - lastTime_30s >= PERIOD_30S)
  {
    lastTime_30s = millis_present;
    valueSensor();        // T1: Read sensors every 30s
    publishSensorData();  // T2: Send to MQTT (V1, V2, V3)
  }
}
