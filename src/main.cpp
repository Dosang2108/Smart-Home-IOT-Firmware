#include <Arduino.h>
#include <global_var.hpp>
#include <MQTT.hpp>
#include <FSM_SETTING.hpp>
#include <display.hpp>
#include <sensors.hpp>
#include <sensor_dashboard.hpp>
#include <control.hpp>
#include <messages.hpp>

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
}

void loop()
{
  millis_update();
  mqttLoop();
  handleSensorDashboard();
  // === 100ms tasks: PIR sensor + IR remote (T6, T7) ===
  if (millis_present - lastTime_100ms >= PERIOD_100MS)
  {
    lastTime_100ms = millis_present;
    led_rgb_tick();       
    readPIR();            
    handleIRRemote();     
    handlePIRControl();   
    handleDoorServo();    
    checkFSMTimeout();    
  }

  // === 1S tasks: Cập nhật giao diện LCD === (MỚI)
  if (millis_present - lastTime_1s >= PERIOD_1S)
  {
    lastTime_1s = millis_present;
    lcdDisplaySensors();  // Chỉ cập nhật màn hình 1 giây/lần để tránh lag UI
  }

  // === 30s tasks: Sensor read + MQTT publish (T1, T2) ===
  if (millis_present - lastTime_30s >= PERIOD_30S)
  {
    lastTime_30s = millis_present;
    valueSensor();        
    publishSensorData();  
  }
}