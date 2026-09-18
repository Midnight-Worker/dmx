#include <Arduino.h>
#include <IRremote.hpp>
#include <SoftwareSerial.h>
#include <util/atomic.h>

// ============================================================
// Hardware
// ============================================================

#define IR_SEND_PIN      5

#define DEBUG_RX_PIN    12
#define DEBUG_TX_PIN    11

#define RS485_DIR_PIN    2

SoftwareSerial debugSerial(DEBUG_RX_PIN, DEBUG_TX_PIN);

// ============================================================
// DMX
// ============================================================

volatile uint16_t dmxSlot = 0;
volatile bool inFrame = false;

volatile uint8_t dmx1 = 0;
volatile uint8_t dmx2 = 0;
volatile uint8_t dmx3 = 0;

volatile bool newDMX = false;

// ============================================================
// IR Codes
// ============================================================

#define IR_ADDRESS 0xEF00

#define CMD_ON          0x00
#define CMD_OFF         0x01

#define CMD_DIM_UP      0x02
#define CMD_DIM_DOWN    0x03

#define CMD_RED         0x04
#define CMD_GREEN       0x05
#define CMD_BLUE        0x06

#define CMD_COLOR_1     0x08
#define CMD_COLOR_2     0x09
#define CMD_COLOR_3     0x0A
#define CMD_COLOR_4     0x0C
#define CMD_TURQUOISE   0x0D
#define CMD_ORANGE      0x0E

#define CMD_CCT_WARMER  0x0F
#define CMD_WARM_WHITE  0x10
#define CMD_NEUTRAL     0x11
#define CMD_COLD_WHITE  0x12
#define CMD_CCT_COLDER  0x13

// ============================================================
// Einstellungen
// ============================================================

#define BRIGHTNESS_LEVELS       16
#define CCT_LEVELS              16

#define BRIGHTNESS_HOME_STEPS   20

#define IR_INTERVAL_MS          120

#define COLOR_OFF_THRESHOLD     10

// ============================================================
// Farben auf DMX CH1
// ============================================================

const uint8_t colorCommands[] =
{
    CMD_RED,
    CMD_ORANGE,
    CMD_COLOR_1,
    CMD_COLOR_2,
    CMD_GREEN,
    CMD_TURQUOISE,
    CMD_BLUE,
    CMD_COLOR_3,
    CMD_COLOR_4
};

#define COLOR_COUNT \
    (sizeof(colorCommands) / sizeof(colorCommands[0]))

// ============================================================
// Zustand
// ============================================================

enum LampMode
{
    MODE_UNKNOWN,
    MODE_COLOR,
    MODE_WHITE
};

LampMode lampMode = MODE_UNKNOWN;

bool lampOn = false;

// ------------------------------------------------------------
// Farbe
// ------------------------------------------------------------

uint8_t currentColor = 255;
bool pendingColor = false;

// ------------------------------------------------------------
// Helligkeit
// ------------------------------------------------------------

uint8_t currentBrightness = 0;
uint8_t targetBrightness = 0;

bool brightnessSynced = false;

uint8_t brightnessHomeRemaining = 0;

// ------------------------------------------------------------
// CCT
// ------------------------------------------------------------

uint8_t currentCCT = 0;
uint8_t targetCCT = 0;

bool cctSynced = false;
bool pendingWhiteHome = false;

// ------------------------------------------------------------
// letzte DMX Rohwerte
// ------------------------------------------------------------

uint8_t oldDMX1 = 0;
uint8_t oldDMX2 = 0;
uint8_t oldDMX3 = 0;

bool firstDMX = true;

// ------------------------------------------------------------

unsigned long lastIR = 0;

// ============================================================
// DMX initialisieren
// ============================================================

void initDMX()
{
    pinMode(RS485_DIR_PIN, OUTPUT);

    // MAX485 auf Empfang
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

// ============================================================
// DMX RX Interrupt
// ============================================================

ISR(USART_RX_vect)
{
    uint8_t status = UCSR0A;
    uint8_t data = UDR0;

    // BREAK
    if (status & (1 << FE0))
    {
        dmxSlot = 0;
        inFrame = true;
        return;
    }

    if (!inFrame)
        return;

    // Startcode
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

    if (dmxSlot == 1)
    {
        dmx1 = data;
    }
    else if (dmxSlot == 2)
    {
        dmx2 = data;
    }
    else if (dmxSlot == 3)
    {
        dmx3 = data;
        newDMX = true;
    }

    dmxSlot++;
}

// ============================================================
// Helfer
// ============================================================

uint8_t mapToLevel(uint8_t value, uint8_t levels)
{
    return ((uint16_t)value * (levels - 1)) / 255;
}

uint8_t mapColor(uint8_t value)
{
    // 0..9 = AUS
    if (value < COLOR_OFF_THRESHOLD)
        return 255;

    uint16_t scaled =
        value - COLOR_OFF_THRESHOLD;

    uint16_t range =
        256 - COLOR_OFF_THRESHOLD;

    uint16_t index =
        (scaled * COLOR_COUNT) / range;

    if (index >= COLOR_COUNT)
        index = COLOR_COUNT - 1;

    return index;
}

// ============================================================
// IR senden
// ============================================================

void sendIR(uint8_t command)
{
    debugSerial.print(F("IR -> 0x"));

    if (command < 0x10)
        debugSerial.print('0');

    debugSerial.println(command, HEX);

    IrSender.sendNEC(
        IR_ADDRESS,
        command,
        0
    );
}

// ============================================================
// Lampe EIN
// ============================================================

void turnLampOn()
{
    if (lampOn)
        return;

    debugSerial.println(F("LAMPE EIN"));

    sendIR(CMD_ON);

    lampOn = true;
    lastIR = millis();

    // Nach jedem Einschalten Helligkeit neu synchronisieren
    brightnessSynced = false;

    brightnessHomeRemaining =
        BRIGHTNESS_HOME_STEPS;

    debugSerial.println(
        F("DIM HOME nach Einschalten")
    );
}

// ============================================================
// Lampe AUS
// ============================================================

void turnLampOff()
{
    if (!lampOn)
        return;

    debugSerial.println(F("LAMPE AUS"));

    sendIR(CMD_OFF);

    lampOn = false;

    pendingColor = false;
    pendingWhiteHome = false;

    lampMode = MODE_UNKNOWN;
    cctSynced = false;

    lastIR = millis();
}

// ============================================================
// DMX auswerten
// ============================================================

void processDMX()
{
    uint8_t ch1;
    uint8_t ch2;
    uint8_t ch3;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ch1 = dmx1;
        ch2 = dmx2;
        ch3 = dmx3;

        newDMX = false;
    }

    // ========================================================
    // Erstes gültiges DMX Paket
    // ========================================================

    if (firstDMX)
    {
        firstDMX = false;

        oldDMX1 = ch1;
        oldDMX2 = ch2;
        oldDMX3 = ch3;

        currentColor =
            mapColor(ch1);

        targetBrightness =
            mapToLevel(
                ch2,
                BRIGHTNESS_LEVELS
            );

        targetCCT =
            mapToLevel(
                ch3,
                CCT_LEVELS
            );

        debugSerial.println();
        debugSerial.println(F("DMX erkannt"));

        if (currentColor != 255)
        {
            turnLampOn();

            pendingColor = true;
        }

        return;
    }

    // ========================================================
    // CH1 = Farbe / EIN / AUS
    // ========================================================

    uint8_t newColor =
        mapColor(ch1);

    // --------------------------------------------------------
    // OFF
    // --------------------------------------------------------

    if (newColor == 255)
    {
        if (lampOn)
            turnLampOff();

        currentColor = 255;
    }

    // --------------------------------------------------------
    // EIN + Farbe
    // --------------------------------------------------------

    else
    {
        if (!lampOn)
        {
            turnLampOn();

            currentColor = newColor;
            pendingColor = true;

            debugSerial.print(F("CH1 EIN / Farbe -> "));
            debugSerial.println(currentColor);
        }

        else if (newColor != currentColor)
        {
            currentColor = newColor;
            pendingColor = true;

            debugSerial.print(F("CH1 Farbe -> "));
            debugSerial.println(currentColor);
        }
    }

    // ========================================================
    // CH2 = Helligkeit
    // ========================================================

    uint8_t newBrightness =
        mapToLevel(
            ch2,
            BRIGHTNESS_LEVELS
        );

    if (newBrightness != targetBrightness)
    {
        targetBrightness = newBrightness;

        debugSerial.print(F("CH2 DIM -> "));
        debugSerial.println(targetBrightness);
    }

    // ========================================================
    // CH3 = CCT
    // ========================================================

    uint8_t newCCT =
        mapToLevel(
            ch3,
            CCT_LEVELS
        );

    if (newCCT != targetCCT)
    {
        targetCCT = newCCT;

        debugSerial.print(F("CH3 CCT -> "));
        debugSerial.println(targetCCT);

        // Wechsel in Weißmodus nur wenn Lampe an ist
        // und wir noch nicht im Weißmodus sind
        if (
            lampOn &&
            lampMode != MODE_WHITE
        )
        {
            pendingWhiteHome = true;

            debugSerial.println(
                F("WEISSMODUS angefordert")
            );
        }
    }

    oldDMX1 = ch1;
    oldDMX2 = ch2;
    oldDMX3 = ch3;
}

// ============================================================
// Lampen State Machine
// ============================================================

void processLamp()
{
    if (!lampOn)
        return;

    if (
        millis() - lastIR <
        IR_INTERVAL_MS
    )
    {
        return;
    }

    // ========================================================
    // 1. Farbe
    // ========================================================

    if (pendingColor)
    {
        sendIR(
            colorCommands[currentColor]
        );

        pendingColor = false;

        lampMode = MODE_COLOR;

        cctSynced = false;

        lastIR = millis();

        return;
    }

    // ========================================================
    // 2. Weißmodus
    // ========================================================

    if (pendingWhiteHome)
    {
        debugSerial.println(
            F("WEISS -> Warmweiss Referenz")
        );

        sendIR(CMD_WARM_WHITE);

        pendingWhiteHome = false;

        lampMode = MODE_WHITE;

        currentCCT = 0;
        cctSynced = true;

        lastIR = millis();

        return;
    }

    // ========================================================
    // 3. Helligkeit einmal auf Minimum synchronisieren
    // ========================================================

    if (brightnessHomeRemaining > 0)
    {
        sendIR(CMD_DIM_DOWN);

        brightnessHomeRemaining--;

        lastIR = millis();

        if (brightnessHomeRemaining == 0)
        {
            currentBrightness = 0;
            brightnessSynced = true;

            debugSerial.println(
                F("DIM synchronisiert")
            );
        }

        return;
    }

    // ========================================================
    // 4. Helligkeit nachführen
    // ========================================================

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

    // ========================================================
    // 5. CCT nachführen
    // ========================================================

    if (
        lampMode == MODE_WHITE &&
        cctSynced
    )
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

// ============================================================
// SETUP
// ============================================================

void setup()
{
    debugSerial.begin(9600);

    delay(500);

    debugSerial.println();
    debugSerial.println(F("MW DMX -> IR v5"));
    debugSerial.println(F("================"));
    debugSerial.println(F("CH1 0..9 = AUS"));
    debugSerial.println(F("CH1 10..255 = Farbe"));
    debugSerial.println(F("CH2 = Helligkeit"));
    debugSerial.println(F("CH3 = Warm/Kalt"));
    debugSerial.println();

    IrSender.begin(IR_SEND_PIN);

    initDMX();

    sei();

    debugSerial.println(F("Bereit."));
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    if (newDMX)
        processDMX();

    processLamp();
}
