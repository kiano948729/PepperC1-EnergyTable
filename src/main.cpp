#include <Arduino.h>
#include "rfid/RFIDManager.h"
#include "rfid/SerialHandler.h"
#include "leds/LedManager.h"

HardwareSerial PepperSerial(2);

RFIDManager rfid(PepperSerial);
SerialHandler output;

SectionComets leds(14, 300, 80); // Data pin, amount of leds, max brightness

void setup()
{
  Serial.begin(115200);
  PepperSerial.begin(115200, SERIAL_8N1, 16, 17);

  output.begin();
  rfid.begin();

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
  rfid.update();

  RFIDEvent event;
  while (rfid.getEvent(event))
  {
    output.handleEvent(event);
  }

  leds.update();
}