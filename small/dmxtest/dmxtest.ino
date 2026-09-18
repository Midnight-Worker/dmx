#include <Arduino.h>
#include <SoftwareSerial.h>
#include <util/atomic.h>

#define DEBUG_RX_PIN 12
#define DEBUG_TX_PIN 11

#define RS485_DIR_PIN 2

SoftwareSerial debugSerial(DEBUG_RX_PIN, DEBUG_TX_PIN);

volatile uint16_t dmxSlot = 0;
volatile bool inFrame = false;

volatile uint8_t dmx1 = 0;
volatile uint8_t dmx2 = 0;
volatile uint8_t dmx3 = 0;

volatile uint32_t frameCounter = 0;


// --------------------------------------------------
// DMX initialisieren
// --------------------------------------------------

void initDMX()
{
    pinMode(RS485_DIR_PIN, OUTPUT);

    // DE + /RE gebrückt:
    // LOW = Empfangen
    digitalWrite(RS485_DIR_PIN, LOW);

    // 250000 Baud bei 16 MHz
    UBRR0H = 0;
    UBRR0L = 3;

    UCSR0A = 0;

    // Receiver + RX Interrupt
    UCSR0B =
        (1 << RXEN0) |
        (1 << RXCIE0);

    // 8 Datenbits, 2 Stopbits
    UCSR0C =
        (1 << UCSZ01) |
        (1 << UCSZ00) |
        (1 << USBS0);
}


// --------------------------------------------------
// UART RX Interrupt
// --------------------------------------------------

ISR(USART_RX_vect)
{
    uint8_t status = UCSR0A;
    uint8_t data = UDR0;

    // DMX BREAK -> Framing Error
    if (status & (1 << FE0))
    {
        dmxSlot = 0;
        inFrame = true;
        return;
    }

    if (!inFrame)
        return;

    // erstes Byte nach BREAK = Startcode
    if (dmxSlot == 0)
    {
        if (data != 0)
        {
            inFrame = false;
            return;
        }

        dmxSlot = 1;
        return;
    }

    // DMX Kanal 1
    if (dmxSlot == 1)
        dmx1 = data;

    // DMX Kanal 2
    else if (dmxSlot == 2)
        dmx2 = data;

    // DMX Kanal 3
    else if (dmxSlot == 3)
    {
        dmx3 = data;
        frameCounter++;
    }

    dmxSlot++;
}


// --------------------------------------------------

void setup()
{
    debugSerial.begin(9600);

    delay(500);

    debugSerial.println();
    debugSerial.println(F("MW DMX RX TEST"));
    debugSerial.println(F("=============="));
    debugSerial.println(F("CH1 CH2 CH3"));

    initDMX();

    sei();
}


// --------------------------------------------------

void loop()
{
    static uint32_t lastPrint = 0;

    if (millis() - lastPrint >= 500)
    {
        lastPrint = millis();

        uint8_t a, b, c;
        uint32_t frames;

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            a = dmx1;
            b = dmx2;
            c = dmx3;
            frames = frameCounter;
        }

        debugSerial.print(F("Frames="));
        debugSerial.print(frames);

        debugSerial.print(F("   CH1="));
        debugSerial.print(a);

        debugSerial.print(F("   CH2="));
        debugSerial.print(b);

        debugSerial.print(F("   CH3="));
        debugSerial.println(c);
    }
}
