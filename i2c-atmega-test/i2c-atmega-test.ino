#include <Wire.h>

constexpr uint8_t I2C_ADDRESS = 0x11;

volatile uint8_t lastCommand = 0;
volatile uint8_t counter = 0;

void receiveEvent(int byteCount)
{
    if (byteCount > 0)
        lastCommand = Wire.read();

    while (Wire.available())
        Wire.read();
}

void requestEvent()
{
    uint8_t answer[3];

    answer[0] = 2;              // Geräte-ID IR-Empfänger
    answer[1] = lastCommand;
    answer[2] = counter++;

    Wire.write(answer, 3);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    // Nano als I2C-Slave 0x11 starten
    Wire.begin(I2C_ADDRESS);

    // Interne 5-V-Pull-ups auf A4/A5 abschalten
    PORTC &= ~((1 << PC4) | (1 << PC5));

    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop()
{
    // Zweimal blinken = Geräte-ID 2
    for (uint8_t i = 0; i < 2; i++)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);

        digitalWrite(LED_BUILTIN, LOW);
        delay(150);
    }

    delay(1000);
}
