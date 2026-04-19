#include <Arduino.h>
#include <global_var.hpp>
#include <MQTT.hpp>
#include <FSM_SETTING.hpp>
#include <display.hpp>
#include <sensors.hpp>
#include <sensor_dashboard.hpp>
#include <control.hpp>
#include <messages.hpp>
#include <app_rtos.hpp>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void setup()
{
  Serial.begin(115200);

  // Initialize pins
  pinSetup();

  // Scan I2C bus to detect LCD address
  scanI2CBus();

  // Initialize NeoPixel
  NeoPixel.begin();
  NeoPixel.clear();
  NeoPixel.show();

  // LCD init
  initLCD();

  // Connect WiFi + MQTT
  init_Wifi_and_MQTT();

  lcd.setCursor(0, 1);
  lcd.print(MSG_WIFI_CONNECTED);

  // NTP time sync (GMT+7)
  initNTP();
  initDHT20();
  initServo();
  initIR();

  // Register structured MQTT channels
  registerChannels();

  // Start local web dashboard
  initSensorDashboard();

  // Hiển thị trạng thái khởi động gọn gàng không delay blocking
  lcd.clear();
  if (isMqttConnected()) {
    lcd.setCursor(0, 0);
    lcd.print(MSG_MQTT_CONNECTED);
    lcd.setCursor(0, 1);
    lcd.print(MSG_SYSTEM_READY);
  } else {
    lcd.setCursor(0, 0);
    lcd.print(MSG_SYSTEM_OK);
  }

  // Khởi tạo đọc cảm biến lần đầu
  valueSensor();
  lcdDisplaySensors();

  // Start RTOS application tasks
  appRtosStart();
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}