#include <Arduino.h>
#include <Wire.h>

// Seeed Studio XIAO ESP32-C6
constexpr int SDA_PIN = 22;   // XIAO D4
constexpr int SCL_PIN = 23;   // XIAO D5

struct Slave
{
    uint8_t address;
    uint8_t expectedId;
    const char* name;
};

const Slave slaves[] =
{
    {0x10, 1, "IR-Sender"},
    {0x11, 2, "IR-Empfaenger"},
    {0x12, 3, "DMX-Controller"}
};

constexpr size_t SLAVE_COUNT =
    sizeof(slaves) / sizeof(slaves[0]);

bool i2cReady = false;
uint8_t commandCounter = 0;

void testSlave(const Slave& slave)
{
    Serial.printf(
        "%-15s 0x%02X: ",
        slave.name,
        slave.address
    );

    Wire.beginTransmission(slave.address);
    Wire.write(commandCounter);

    uint8_t error = Wire.endTransmission();

    if (error != 0)
    {
        Serial.printf("keine Antwort, Fehler %u\n", error);
        return;
    }

    delay(5);

    uint8_t received = Wire.requestFrom(
        slave.address,
        static_cast<uint8_t>(3)
    );

    if (received != 3)
    {
        Serial.printf(
            "nur %u von 3 Bytes empfangen\n",
            received
        );

        while (Wire.available())
            Wire.read();

        return;
    }

    uint8_t id = Wire.read();
    uint8_t command = Wire.read();
    uint8_t counter = Wire.read();

    Serial.printf(
        "OK | ID=%u | Kommando=%u | Zaehler=%u",
        id,
        command,
        counter
    );

    if (id != slave.expectedId)
        Serial.print(" | FALSCHE ID");

    if (command != commandCounter)
        Serial.print(" | FALSCHES KOMMANDO");

    Serial.println();
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" XIAO ESP32-C6 I2C-Test");
    Serial.println("================================");
    Serial.println("SDA: D4 / GPIO22");
    Serial.println("SCL: D5 / GPIO23");

    pinMode(SDA_PIN, INPUT);
    pinMode(SCL_PIN, INPUT);
    delay(20);

    Serial.printf(
        "Vor I2C: SDA=%s SCL=%s\n",
        digitalRead(SDA_PIN) ? "HIGH" : "LOW",
        digitalRead(SCL_PIN) ? "HIGH" : "LOW"
    );

    i2cReady = Wire.begin(
        SDA_PIN,
        SCL_PIN,
        50000
    );

    Serial.printf(
        "Wire.begin(): %s\n",
        i2cReady ? "OK" : "FEHLER"
    );
}

void loop()
{
    if (!i2cReady)
    {
        Serial.println("I2C nicht gestartet!");
        delay(2000);
        return;
    }

    commandCounter++;

    Serial.println();
    Serial.printf(
        "--- Abfrage %u ---\n",
        commandCounter
    );

    Serial.printf(
        "SDA=%s SCL=%s\n",
        digitalRead(SDA_PIN) ? "HIGH" : "LOW",
        digitalRead(SCL_PIN) ? "HIGH" : "LOW"
    );

    for (size_t i = 0; i < SLAVE_COUNT; i++)
        testSlave(slaves[i]);

    delay(2000);
}
