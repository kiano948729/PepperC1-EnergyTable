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

  int section = leds.addSectionRange(0, 300);

  for (int i = 0; i < 20; ++i)
  {
    CometOptions comet{};
    comet.color = {
        (uint8_t)random(10, 81),
        (uint8_t)random(10, 81),
        (uint8_t)random(10, 81)};
    comet.direction = random(0, 2) == 0 ? -1 : 1;
    comet.speed = random(1, 7) * 2.0f;
    comet.gap = random(0, 5);
    comet.position = random(0, 300);
    comet.showHead = false;

    leds.addComet(section, comet);
  }

  // int section = leds.addSectionRange(0, 10);

  // int section2 = leds.addSectionRange(21, 34);
  // int section3 = leds.addSectionRange(38, 50);

  // CometOptions cometEnergy;
  // cometEnergy.color = {20, 20, 80};
  // cometEnergy.direction = -1;
  // cometEnergy.speed = 16.0f;
  // cometEnergy.tailLength = 3;
  // cometEnergy.gap = 0;
  // cometEnergy.position = 0.0f;
  // cometEnergy.showHead = false;
  // cometEnergy.paused = false;

  // CometOptions cometHeat;
  // cometHeat.color = {80, 10, 0};
  // cometHeat.direction = -1;
  // cometHeat.speed = 2.0f;
  // cometHeat.tailLength = 3;
  // cometHeat.gap = 3;
  // cometHeat.position = 0.0f;
  // cometHeat.showHead = false;
  // cometHeat.paused = false;

  // leds.addComet(section, cometEnergy);
  // leds.addComet(section, cometHeat);

  // cometEnergy.direction = 1;

  // leds.addComet(section2, cometEnergy);
  // leds.addComet(section3, cometHeat);

  output.message("ENERGY TABLE READY");
}

int direction = 1;
int brightnessIndex = 0;

void loop()
{
  rfid.update();

  RFIDEvent event;
  while (rfid.getEvent(event))
  {
    output.handleEvent(event);
  }

  leds.update();

  // leds.blink({(brightnessIndex * random()), (brightnessIndex * random()), (brightnessIndex * random())}, -1);
  // brightnessIndex += direction;
  // if (brightnessIndex >= 80 || brightnessIndex <= 0)
  // {
  //   direction *= -1;
  // }
}