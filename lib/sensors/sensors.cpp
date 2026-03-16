#include "sensors.hpp"

DHT20 DHT;

bool initDHT20()
{
  if (!DHT.begin()) {
    Serial.println("DHT20 init failed");
    dhtDataValid = false;
    return false;
  }
  Serial.println("DHT20 init success");
  dhtDataValid = true;
  return true;
}

void valueSensor()
{
  readDHT20();
  readLight();
  readPIR();
  
}

void readDHT20()
{
  if (DHT.read() == DHT20_OK) {
    Value_Humidity = round(DHT.getHumidity() * 100) / 100.0;
    Value_Temperature = round(DHT.getTemperature() * 100) / 100.0;
    dhtDataValid = true;
  } else {
    dhtDataValid = false;
    Serial.println("DHT20 read failed");
  }
}

// void readSoilMoisture()
// {
//   int raw = analogRead(soilMoisturePin);
//   // Quy đổi: 3500 = Khô (0%), 1200 = Ướt đẫm (100%)
//   Value_SoilMoisture = map(raw, 3500, 1200, 0, 100);
//   Value_SoilMoisture = constrain(Value_SoilMoisture, 0, 100); // Chặn không cho vượt quá 100% hoặc bị âm
// }

void readLight()
{
  int raw = analogRead(light);
  // Quy đổi: 4095 = Tối (0%), 500 = Sáng chói (100%)
  Value_Light = map(raw, 4095, 500, 0, 100);
  Value_Light = constrain(Value_Light, 0, 100);
}

void readPIR()
{
  pirDetected = digitalRead(PIR_PIN);
}