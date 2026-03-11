#include "display.hpp"

LiquidCrystal_I2C lcd(0x21, 16, 2);

void lcdDisplay()
{
  // lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(Value_Temperature, 1);
  // lcd.print(DHT.getTemperature(), 1);
  lcd.print("C");

  lcd.print(" L:");
  lcd.print(Value_Light);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("H:");
  lcd.print(Value_Humidity, 1);
  // lcd.print(DHT.getHumidity(), 1);
  lcd.print("% ");

  lcd.print("S:");
  lcd.print(Value_SoilMoisture);
  lcd.print("% ");
  delay(1000);
}