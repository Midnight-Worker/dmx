#include <Arduino.h>
#include <SoftwareSerial.h>
#include <IRremote.hpp>

#define IR_RECEIVE_PIN 10

// Debug:
// RX = D12
// TX = D11 -> USB-UART RX
SoftwareSerial Debug(12, 11);

void setup()
{
    Debug.begin(9600);

    IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

    Debug.println();
    Debug.println("IR Scanner gestartet");
    Debug.println("Fernbedienung druecken...");
}

void loop()
{
    if (IrReceiver.decode())
    {
        Debug.println();
        Debug.println("IR empfangen:");

        Debug.print("Protokoll: ");
        Debug.println(getProtocolString(IrReceiver.decodedIRData.protocol));

        Debug.print("Adresse: 0x");
        Debug.println(IrReceiver.decodedIRData.address, HEX);

        Debug.print("Kommando: 0x");
        Debug.println(IrReceiver.decodedIRData.command, HEX);

        Debug.print("Raw: 0x");
        Debug.println(IrReceiver.decodedIRData.decodedRawData, HEX);

        Debug.print("Flags: 0x");
        Debug.println(IrReceiver.decodedIRData.flags, HEX);

        Debug.println("----------------");

        IrReceiver.resume();
    }
}
