#include <Arduino.h>
#include <IRremote.hpp>

#define IR_SEND_PIN 5

#define IR_ADDRESS 0xEF00
#define CMD_ON     0x01

void setup()
{
    IrSender.begin(IR_SEND_PIN);

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);

    // Dein bekannter Einschaltbefehl
    IrSender.sendNEC(IR_ADDRESS, CMD_ON, 0);

    delay(100);

    digitalWrite(LED_BUILTIN, LOW);

    delay(900);
}
