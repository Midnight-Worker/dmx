#include <Arduino.h>
#line 1 "C:\\Users\\maikt\\Desktop\\dmx\\stage\\hallo_atmega\\hallo_atmega.ino"
#include <SoftwareSerial.h>

// RX, TX
SoftwareSerial debugSerial(12, 11);

#line 6 "C:\\Users\\maikt\\Desktop\\dmx\\stage\\hallo_atmega\\hallo_atmega.ino"
void setup();
#line 13 "C:\\Users\\maikt\\Desktop\\dmx\\stage\\hallo_atmega\\hallo_atmega.ino"
void loop();
#line 6 "C:\\Users\\maikt\\Desktop\\dmx\\stage\\hallo_atmega\\hallo_atmega.ino"
void setup()
{
    debugSerial.begin(9600);
    debugSerial.println("Hallo Welt!");
    debugSerial.println("ATmega328P ist gestartet.");
}

void loop()
{
    static unsigned long counter = 0;

    debugSerial.print("Zaehler: ");
    debugSerial.println(counter++);

    delay(1000);
}

