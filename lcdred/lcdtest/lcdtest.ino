#include <LiquidCrystal.h>

// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    lcd.begin(20, 4);
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("LCD TEST");

    lcd.setCursor(0, 1);
    lcd.print("Arduino Nano");

    lcd.setCursor(0, 2);
    lcd.print("LiquidCrystal OK");

    lcd.setCursor(0, 3);
    lcd.print("ABC123");
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);

    digitalWrite(LED_BUILTIN, LOW);
    delay(750);
}
