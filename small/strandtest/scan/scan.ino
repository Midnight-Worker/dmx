#include <Arduino.h>
#include <SoftwareSerial.h>

#define DMX_ENABLE 2

// Debug über ICSP:
// D12 = RX, unbenutzt
// D11 = TX -> USB-UART RX
SoftwareSerial Debug(12, 11);

void setup()
{
    Debug.begin(9600);

    pinMode(DMX_ENABLE, OUTPUT);

    // MAX485 EMPFANG
    // DE = LOW
    // /RE = LOW
    digitalWrite(DMX_ENABLE, LOW);

    // DMX Hardware-UART
    Serial.begin(250000, SERIAL_8N2);

    delay(500);

    Debug.println();
    Debug.println("RAW DMX TEST");
}

void loop()
{
    static unsigned long last = 0;
    static unsigned long bytes = 0;

    while (Serial.available())
    {
        Serial.read();
        bytes++;
    }

    if (millis() - last >= 1000)
    {
        last = millis();

        Debug.print("Bytes/s: ");
        Debug.println(bytes);

        bytes = 0;
    }
}
