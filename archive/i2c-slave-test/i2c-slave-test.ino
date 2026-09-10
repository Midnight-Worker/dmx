#include <Wire.h>

// Für jeden ATmega ändern:
#define I2C_ADDRESS 0x12
#define DEVICE_ID   3

volatile uint8_t lastCommand = 0;
volatile uint8_t requestCounter = 0;

void receiveEvent(int byteCount)
{
    if (byteCount > 0)
        lastCommand = Wire.read();

    while (Wire.available())
        Wire.read();
}

void requestEvent()
{
    uint8_t response[3];

    response[0] = DEVICE_ID;
    response[1] = lastCommand;
    response[2] = requestCounter++;

    Wire.write(response, sizeof(response));
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // ATmega328P:
    // A4 = SDA
    // A5 = SCL
    Wire.begin(I2C_ADDRESS);

    // Interne Pull-ups nach 5 V abschalten
    PORTC &= ~((1 << PC4) | (1 << PC5));

    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

void loop()
{
    // Anzahl der Blinksignale entspricht DEVICE_ID
    for (uint8_t i = 0; i < DEVICE_ID; i++)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);

        digitalWrite(LED_BUILTIN, LOW);
        delay(150);
    }

    delay(1000);
}
