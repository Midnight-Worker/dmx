#include <Arduino.h>
#include <SoftwareSerial.h>

#define DMX_ENABLE 2

// Debug-UART:
// D12 = RX  (momentan unbenutzt)
// D11 = TX  -> USB-UART RX
SoftwareSerial Debug(12, 11);

uint8_t channels[6] = {0};

bool waitingForStartCode = false;
uint16_t channelNumber = 0;

void setupDMX()
{
    pinMode(DMX_ENABLE, OUTPUT);

    // MAX485 auf EMPFANG:
    // DE  = LOW
    // /RE = LOW
    digitalWrite(DMX_ENABLE, LOW);

    // 250000 Baud bei 16 MHz
    UBRR0H = 0;
    UBRR0L = 3;

    UCSR0A = 0;

    // RX einschalten
    UCSR0B = (1 << RXEN0);

    // 8 Datenbits, 2 Stopbits
    UCSR0C =
        (1 << USBS0) |
        (1 << UCSZ01) |
        (1 << UCSZ00);
}

void printChannels()
{
    // ANSI: Bildschirm löschen + Cursor nach oben links
    Debug.print("\033[2J");
    Debug.print("\033[H");

    Debug.println("DMX Scanner");
    Debug.println("-----------");

    Debug.print("Kanal 1: ");
    Debug.println(channels[0]);

    Debug.print("Kanal 2: ");
    Debug.println(channels[1]);

    Debug.print("Kanal 3: ");
    Debug.println(channels[2]);

    Debug.print("Kanal 4: ");
    Debug.println(channels[3]);

    Debug.print("Kanal 5: ");
    Debug.println(channels[4]);

    Debug.print("Kanal 6: ");
    Debug.println(channels[5]);
}

void setup()
{
    Debug.begin(9600);

    delay(500);

    Debug.println();
    Debug.println("DMX Scanner gestartet");

    setupDMX();
}

void loop()
{
    if (UCSR0A & (1 << RXC0))
    {
        // Status unbedingt VOR UDR0 lesen
        uint8_t status = UCSR0A;
        uint8_t data = UDR0;

        // BREAK erkannt
        if (status & (1 << FE0))
        {
            waitingForStartCode = true;
            channelNumber = 0;
            return;
        }

        // nächstes Byte nach BREAK = DMX Startcode
        if (waitingForStartCode)
        {
            waitingForStartCode = false;

            if (data == 0)
            {
                channelNumber = 1;
            }
            else
            {
                channelNumber = 0;
            }

            return;
        }

        // DMX-Kanäle lesen
        if (channelNumber >= 1)
        {
            if (channelNumber <= 6)
            {
                channels[channelNumber - 1] = data;
            }

            channelNumber++;

            // Kanal 1-6 komplett
            if (channelNumber == 7)
            {
                printChannels();

                // auf nächsten BREAK warten
                channelNumber = 0;
            }
        }
    }
}
