#include <Arduino.h>
#include <LittleFS.h>

#include "rfid/RFIDManager.h"
#include "rfid/SerialHandler.h"
#include "storage/CSVManager.h"
#include "leds/LedManager.h"

// Twee afzonderlijke UART-poorten
HardwareSerial PepperSerial1(2);
HardwareSerial PepperSerial2(1);

// RFID managers
RFIDManager rfid1(PepperSerial1);
RFIDManager rfid2(PepperSerial2);

// Centrale managers
SerialHandler output;
CSVManager csv;

bool csvReady = false;

// USB-serial synchronisatie
String syncInputLine;

// LED manager
SectionComets leds(14, 300, 80);

// --------------------------------------------------
// SYNCHRONISATIE: COMPUTER -> ESP32
// --------------------------------------------------

void handleSyncCommand(const String &line)
{
  // Python vraagt of de ESP32 klaar is.
  if (line == "HELLO")
  {
    if (!csvReady)
    {
      Serial.println("SYNC_ERROR\tSTORAGE_NOT_READY");
      return;
    }

    Serial.println("SYNC_READY");

    // Stuur alle bestaande tags naar de computer.
    csv.sendAll(Serial);
    return;
  }

  if (!line.startsWith("SET\t"))
    return;

  int first = line.indexOf('\t');
  int second = line.indexOf('\t', first + 1);
  int third = line.indexOf('\t', second + 1);

  if (first < 0 || second < 0 || third < 0 || !csvReady)
  {
    Serial.println("SYNC_ERROR\tBAD_COMMAND_OR_STORAGE");
    return;
  }

  String uid = line.substring(first + 1, second);
  String consumption = line.substring(second + 1, third);
  String generation = line.substring(third + 1);

  uid.trim();
  consumption.trim();
  generation.trim();

  if (uid.length() == 0 ||
      uid.indexOf(',') >= 0 ||
      consumption.indexOf(',') >= 0 ||
      generation.indexOf(',') >= 0)
  {
    Serial.println("SYNC_ERROR\tINVALID_FIELD");
    return;
  }

  bool wasAdded = false;

  if (!csv.ensureTag(uid, wasAdded) ||
      !csv.updateTagData(uid, consumption, generation))
  {
    Serial.println("SYNC_ERROR\tWRITE_FAILED");
    return;
  }

  Serial.println("SYNC_OK");
}

// --------------------------------------------------
// SERIELE INPUT VERWERKEN
// --------------------------------------------------

void handleSyncSerial()
{
  while (Serial.available())
  {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r')
    {
      if (syncInputLine.length() > 0)
      {
        handleSyncCommand(syncInputLine);
        syncInputLine = "";
      }
    }
    else if (syncInputLine.length() < 240)
    {
      syncInputLine += c;
    }
    else
    {
      syncInputLine = "";
      Serial.println("SYNC_ERROR\tLINE_TOO_LONG");
    }
  }
}

// --------------------------------------------------
// RFID EVENTS VERWERKEN
// --------------------------------------------------

void handlePepperEvents(
    RFIDManager &rfid,
    const char *pepperName)
{
  RFIDEvent event;

  while (rfid.getEvent(event))
  {
    output.handleEvent(event, pepperName);

    if (!event.added)
      continue;

    if (!csvReady)
    {
      output.message("CSV ERROR: STORAGE NOT READY");
      continue;
    }

    bool wasAdded = false;

    if (csv.ensureTag(event.uid, wasAdded))
    {
      if (wasAdded)
      {
        output.message(
            "CSV: NEW UID SAVED: " + event.uid);
      }
      else
      {
        output.message(
            "CSV: UID ALREADY EXISTS: " + event.uid);
      }

      TagData data;

      if (csv.getTagData(event.uid, data))
      {
        output.message(
            "CONSUMPTION=" + data.consumption +
            " | GENERATION=" + data.generation);

        // Machineleesbare melding voor Python.
        Serial.println(
            "TAG\t" + data.uid + "\t" +
            data.consumption + "\t" +
            data.generation);
      }
    }
    else
    {
      output.message(
          "CSV ERROR: COULD NOT SAVE UID: " + event.uid);
    }
  }
}

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup()
{
  Serial.begin(115200);

  // Pepper 1
  PepperSerial1.begin(
      115200,
      SERIAL_8N1,
      16,
      17);

  // Pepper 2
  PepperSerial2.begin(
      115200,
      SERIAL_8N1,
      32,
      33);

  output.begin();

  // LittleFS / CSV initialiseren
  output.message("INITIALIZING LITTLEFS...");

  csvReady = csv.begin();

  if (csvReady)
  {
    output.message("LITTLEFS READY");
    output.message("CSV STORAGE READY");
  }
  else
  {
    output.message("ERROR: LITTLEFS MOUNT FAILED");
    output.message("CHECK FILESYSTEM IMAGE");
  }

  // RFID initialiseren
  rfid1.begin();
  rfid2.begin();

  output.message("ENERGY TABLE READY");

  // De synchronisatie start zodra Python HELLO verstuurt.

  output.message("PEPPER 1 CONNECTED");
  output.message("PEPPER 2 CONNECTED");

  // LEDs initialiseren
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
  // CSV-commando's vanaf de computer verwerken.
  handleSyncSerial();

  // Beide RFID-readers uitlezen.
  rfid1.update();
  rfid2.update();

  // Beide Peppers gebruiken dezelfde CSV.
  handlePepperEvents(rfid1, "PEPPER 1");
  handlePepperEvents(rfid2, "PEPPER 2");

  leds.update();
}