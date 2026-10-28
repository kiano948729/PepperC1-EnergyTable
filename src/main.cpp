#include <Arduino.h>
#include "rfid/RFIDManager.h"
#include "rfid/SerialHandler.h"

HardwareSerial PepperSerial(2);

RFIDManager rfid(PepperSerial);
SerialHandler output;

void setup()
{
  Serial.begin(115200);
  PepperSerial.begin(115200, SERIAL_8N1, 16, 17);

  output.begin();
  rfid.begin();

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
}