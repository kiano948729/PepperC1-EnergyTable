#include <Arduino.h>
#include "rfid/RFIDManager.h"
#include "rfid/SerialHandler.h"
#include "leds/LedManager.h"

// Twee afzonderlijke UART-poorten
HardwareSerial PepperSerial1(2);
HardwareSerial PepperSerial2(1);

// Iedere Pepper krijgt een eigen RFIDManager
RFIDManager rfid1(PepperSerial1);
RFIDManager rfid2(PepperSerial2);

SerialHandler output;

SectionComets leds(14, 300, 80); // Data pin, amount of leds, max brightness

void setup()
{
    Serial.begin(115200);

    // Pepper C1 nummer 1
    PepperSerial1.begin(115200, SERIAL_8N1, 16, 17);

    // Pepper C1 nummer 2
    PepperSerial2.begin(115200, SERIAL_8N1, 32, 33);

    output.begin();

    rfid1.begin();
    rfid2.begin();

    output.message("ENERGY TABLE READY");
    output.message("PEPPER 1 CONNECTED");
    output.message("PEPPER 2 CONNECTED");
  leds.begin();

  int section = leds.addSectionRange(0, 30);

  CometOptions comet;
  comet.color = {80, 0, 0};
  comet.direction = 1;
  comet.speed = 2.0f;
  comet.tailLength = 3;
  comet.gap = 0;
  comet.position = 0.0f;
  comet.showHead = false;
  comet.paused = false;

  int cometId = leds.addComet(section, comet);

  output.message("ENERGY TABLE READY");
}

void loop()
{
    // Beide Pepper-modules uitlezen
    rfid1.update();
    rfid2.update();

    RFIDEvent event;

    // Events van Pepper 1
    while (rfid1.getEvent(event))
    {
        output.handleEvent(event, "PEPPER 1");
    }

    // Events van Pepper 2
    while (rfid2.getEvent(event))
    {
        output.handleEvent(event, "PEPPER 2");
    }
}
