#include <Arduino.h>
#include <IRremote.hpp>
#include <SoftwareSerial.h>
#include <util/atomic.h>

// ------------------------------------------------------------
// Hardware
// ------------------------------------------------------------

#define IR_SEND_PIN       5
#define IR_RECEIVE_PIN   10

#define DEBUG_RX_PIN     12   // MISO
#define DEBUG_TX_PIN     11   // MOSI

#define RS485_DIR_PIN     2   // DE + /RE zusammen

SoftwareSerial debugSerial(DEBUG_RX_PIN, DEBUG_TX_PIN);

// ------------------------------------------------------------
// DMX
// ------------------------------------------------------------

#define DMX_START_ADDRESS 1

volatile uint8_t dmxData[3] = {0, 0, 0};

volatile uint16_t dmxSlot = 0;
volatile bool dmxFrame = false;
volatile bool dmxValid = false;

// ------------------------------------------------------------
// IR
// ------------------------------------------------------------

#define IR_ADDRESS 0xEF00

#define CMD_DIM_UP       0x02
#define CMD_DIM_DOWN     0x03

#define CMD_RED          0x04
#define CMD_GREEN        0x05
#define CMD_BLUE         0x06

#define CMD_COLOR_1      0x08
#define CMD_COLOR_2      0x09
#define CMD_COLOR_3      0x0A
#define CMD_COLOR_4      0x0C
#define CMD_TURQUOISE    0x0D
#define CMD_ORANGE       0x0E

#define CMD_CCT_WARMER   0x0F
#define CMD_WARM_WHITE   0x10
#define CMD_NEUTRAL      0x11
#define CMD_COLD_WHITE   0x12
#define CMD_CCT_COLDER   0x13

// ------------------------------------------------------------
// Feinabstimmung
// ------------------------------------------------------------

// erstmal geschätzt, später messen/anpassen
#define BRIGHTNESS_LEVELS      16
#define CCT_LEVELS             16

// genug DIM-DOWN senden, um sicher am Minimum zu landen
#define BRIGHTNESS_HOME_STEPS  24

// Abstand zwischen IR-Kommandos
#define IR_INTERVAL_MS         110

// ------------------------------------------------------------
// Farben für DMX CH1
// ------------------------------------------------------------

const uint8_t colorCommands[] = {
    CMD_RED,
    CMD_ORANGE,
    CMD_COLOR_1,
    CMD_GREEN,
    CMD_COLOR_2,
    CMD_TURQUOISE,
    CMD_COLOR_3,
    CMD_BLUE,
    CMD_COLOR_4
};

#define COLOR_COUNT (sizeof(colorCommands) / sizeof(colorCommands[0]))

// ------------------------------------------------------------
// Zustände
// ------------------------------------------------------------

enum LampMode {
    MODE_UNKNOWN,
    MODE_COLOR,
    MODE_WHITE
};

LampMode lampMode = MODE_UNKNOWN;

uint8_t targetBrightness = 0;
uint8_t currentBrightness = 0;

bool brightnessSynced = false;
uint8_t brightnessHomeRemaining = 0;

uint8_t targetCCT = 0;
uint8_t currentCCT = 0;

bool cctSynced = false;

uint8_t currentColor = 255;

bool pendingColor = false;
bool pendingWhite = false;

unsigned long lastIR = 0;

uint8_t lastDMXColor = 0;
uint8_t lastDMXBrightness = 0;
uint8_t lastDMXCCT = 0;

bool firstDMXFrame = true;

// ------------------------------------------------------------
// DMX UART initialisieren
// 250000 Baud, 8N2
// ------------------------------------------------------------

void initDMX()
{
    pinMode(RS485_DIR_PIN, OUTPUT);

    // MAX485 Empfang
    digitalWrite(RS485_DIR_PIN, LOW);

    // 250000 Baud @ 16 MHz
    UBRR0H = 0;
    UBRR0L = 3;

    UCSR0A = 0;

    UCSR0B =
        (1 << RXEN0) |
        (1 << RXCIE0);

    // 8 Datenbits, 2 Stopbits
    UCSR0C =
        (1 << UCSZ01) |
        (1 << UCSZ00) |
        (1 << USBS0);
}

// ------------------------------------------------------------
// DMX Empfang ISR
// ------------------------------------------------------------

ISR(USART_RX_vect)
{
    uint8_t status = UCSR0A;
    uint8_t data = UDR0;

    // DMX BREAK wird beim AVR als Framing Error sichtbar
    if (status & (1 << FE0))
    {
        dmxSlot = 0;
        dmxFrame = true;
        return;
    }

    if (!dmxFrame)
        return;

    // Slot 0 = Startcode
    if (dmxSlot == 0)
    {
        if (data != 0)
        {
            dmxFrame = false;
            return;
        }

        dmxSlot = 1;
        return;
    }

    // Wir lesen DMX Kanal 1, 2 und 3
    if (dmxSlot >= DMX_START_ADDRESS &&
        dmxSlot < DMX_START_ADDRESS + 3)
    {
        dmxData[dmxSlot - DMX_START_ADDRESS] = data;

        if (dmxSlot == DMX_START_ADDRESS + 2)
            dmxValid = true;
    }

    dmxSlot++;
}

// ------------------------------------------------------------
// Helfer
// ------------------------------------------------------------

uint8_t mapToLevel(uint8_t value, uint8_t levels)
{
    return ((uint16_t)value * (levels - 1)) / 255;
}

uint8_t mapColor(uint8_t value)
{
    uint16_t n = ((uint16_t)value * COLOR_COUNT) / 256;

    if (n >= COLOR_COUNT)
        n = COLOR_COUNT - 1;

    return n;
}

void sendIR(uint8_t command)
{
    debugSerial.print(F("IR 0x"));

    if (command < 0x10)
        debugSerial.print('0');

    debugSerial.println(command, HEX);

    IrSender.sendNEC(
        IR_ADDRESS,
        command,
        0
    );
}

// ------------------------------------------------------------
// DMX Werte auswerten
// ------------------------------------------------------------

void processDMX()
{
    uint8_t ch1;
    uint8_t ch2;
    uint8_t ch3;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ch1 = dmxData[0];
        ch2 = dmxData[1];
        ch3 = dmxData[2];
    }

    // Erstes DMX Paket nur übernehmen
    if (firstDMXFrame)
    {
        firstDMXFrame = false;

        lastDMXColor      = ch1;
        lastDMXBrightness = ch2;
        lastDMXCCT        = ch3;

        currentColor =
            mapColor(ch1);

        targetBrightness =
            mapToLevel(ch2, BRIGHTNESS_LEVELS);

        targetCCT =
            mapToLevel(ch3, CCT_LEVELS);

        debugSerial.println(F("DMX erkannt"));

        return;
    }

    // --------------------------------------------------------
    // CH1 = Farbe
    // --------------------------------------------------------

    uint8_t newColor = mapColor(ch1);

    if (newColor != currentColor)
    {
        currentColor = newColor;

        pendingColor = true;
        pendingWhite = false;

        debugSerial.print(F("COLOR -> "));
        debugSerial.println(currentColor);
    }

    // --------------------------------------------------------
    // CH2 = Helligkeit
    // --------------------------------------------------------

    uint8_t newBrightness =
        mapToLevel(ch2, BRIGHTNESS_LEVELS);

    if (newBrightness != targetBrightness)
    {
        targetBrightness = newBrightness;

        debugSerial.print(F("DIM target -> "));
        debugSerial.println(targetBrightness);

        if (!brightnessSynced)
        {
            brightnessHomeRemaining =
                BRIGHTNESS_HOME_STEPS;
        }
    }

    // Fader ganz unten = Resync
    if (ch2 <= 2 && lastDMXBrightness > 2)
    {
        brightnessSynced = false;

        brightnessHomeRemaining =
            BRIGHTNESS_HOME_STEPS;

        targetBrightness = 0;

        debugSerial.println(F("DIM HOME"));
    }

    // --------------------------------------------------------
    // CH3 = CCT / Weißtemperatur
    // --------------------------------------------------------

    uint8_t newCCT =
        mapToLevel(ch3, CCT_LEVELS);

    if (newCCT != targetCCT)
    {
        targetCCT = newCCT;

        pendingWhite = true;
        pendingColor = false;

        debugSerial.print(F("CCT target -> "));
        debugSerial.println(targetCCT);
    }

    lastDMXColor      = ch1;
    lastDMXBrightness = ch2;
    lastDMXCCT        = ch3;
}

// ------------------------------------------------------------
// Lampensteuerung
// ------------------------------------------------------------

void processLamp()
{
    if (millis() - lastIR < IR_INTERVAL_MS)
        return;

    // --------------------------------------------------------
    // Farbmodus
    // --------------------------------------------------------

    if (pendingColor)
    {
        sendIR(colorCommands[currentColor]);

        lampMode = MODE_COLOR;
        pendingColor = false;

        lastIR = millis();
        return;
    }

    // --------------------------------------------------------
    // Weißmodus
    // --------------------------------------------------------

    if (pendingWhite)
    {
        sendIR(CMD_WARM_WHITE);

        lampMode = MODE_WHITE;

        currentCCT = 0;
        cctSynced = true;

        pendingWhite = false;

        lastIR = millis();
        return;
    }

    // --------------------------------------------------------
    // Helligkeit Resync
    // --------------------------------------------------------

    if (brightnessHomeRemaining > 0)
    {
        sendIR(CMD_DIM_DOWN);

        brightnessHomeRemaining--;

        if (brightnessHomeRemaining == 0)
        {
            currentBrightness = 0;
            brightnessSynced = true;

            debugSerial.println(F("DIM synchronisiert"));
        }

        lastIR = millis();
        return;
    }

    // --------------------------------------------------------
    // Helligkeit fahren
    // --------------------------------------------------------

    if (brightnessSynced)
    {
        if (currentBrightness < targetBrightness)
        {
            sendIR(CMD_DIM_UP);

            currentBrightness++;

            lastIR = millis();
            return;
        }

        if (currentBrightness > targetBrightness)
        {
            sendIR(CMD_DIM_DOWN);

            currentBrightness--;

            lastIR = millis();
            return;
        }
    }

    // --------------------------------------------------------
    // CCT fahren
    // --------------------------------------------------------

    if (lampMode == MODE_WHITE && cctSynced)
    {
        if (currentCCT < targetCCT)
        {
            sendIR(CMD_CCT_COLDER);

            currentCCT++;

            lastIR = millis();
            return;
        }

        if (currentCCT > targetCCT)
        {
            sendIR(CMD_CCT_WARMER);

            currentCCT--;

            lastIR = millis();
            return;
        }
    }
}

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------

void setup()
{
    debugSerial.begin(9600);

    delay(300);

    debugSerial.println();
    debugSerial.println(F("MW DMX -> IR"));
    debugSerial.println(F("================"));
    debugSerial.println(F("DMX CH1 = Farbe"));
    debugSerial.println(F("DMX CH2 = Helligkeit"));
    debugSerial.println(F("DMX CH3 = CCT"));
    debugSerial.println();

    IrSender.begin(IR_SEND_PIN);

    initDMX();

    sei();
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------

void loop()
{
    if (dmxValid)
    {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            dmxValid = false;
        }

        processDMX();
    }

    processLamp();
}
