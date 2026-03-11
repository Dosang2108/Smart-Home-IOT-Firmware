#include "sensors.hpp"

DHT20 DHT;

void valueSensor()
{
  readDHT20();
  readSoilMoisture();
  readLight();
}

void readDHT20()
{
  DHT.read();
  Value_Humidity = round(DHT.getHumidity() * 100) / 100.0;
  Value_Temperature = round(DHT.getTemperature() * 100) / 100.0;
}

void readSoilMoisture()
{
  int soilMoistureValue = analogRead(soilMoisturePin);
  Value_SoilMoisture = soilMoistureValue * 100 / 4095;
}

void readLight()
{
  int lightValue = analogRead(light);
  Value_Light = lightValue * 100 / 4095;
}

void readPIR()
{
  pirDetected = digitalRead(PIR_PIN);
}