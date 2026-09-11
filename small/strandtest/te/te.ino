#include <Arduino.h>

const uint8_t DMX_ENABLE = 2;

// Startcode + 6 DMX-Kanäle
uint8_t dmx[7];

void sendDMX()
{
    // Sicherstellen, dass vorher alles raus ist
    Serial.flush();

    // UART kurz abschalten, damit wir TX manuell für BREAK benutzen können
    Serial.end();

    pinMode(1, OUTPUT);      // TX = D1

    // DMX BREAK
    digitalWrite(1, LOW);
    delayMicroseconds(120);

    // Mark After Break
    digitalWrite(1, HIGH);
    delayMicroseconds(20);

    // DMX UART: 250000 Baud, 8N2
    Serial.begin(250000, SERIAL_8N2);

    // Startcode + Kanäle senden
    Serial.write(dmx, sizeof(dmx));
    Serial.flush();
}

void setup()
{
    pinMode(DMX_ENABLE, OUTPUT);

    // MAX485 auf SENDEN
    digitalWrite(DMX_ENABLE, HIGH);

    // Startcode
    dmx[0] = 0;

    // DMX Kanal 1–6
    dmx[1] = 100;
    dmx[2] = 100;
    dmx[3] = 20;
    dmx[4] = 0;
    dmx[5] = 0;
    dmx[6] = 0;

    Serial.begin(250000, SERIAL_8N2);
}

void loop()
{
    sendDMX();

    // ca. 30–35 Frames/s
    delay(30);
}
