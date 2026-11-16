#include "rfid/SerialHandler.h"

void SerialHandler::begin()
{
}

void SerialHandler::message(const String &text)
{
    Serial.println(text);
}

void SerialHandler::handleEvent(
    const RFIDEvent &event,
    const char* pepperName
)
{
    Serial.print("[");
    Serial.print(pepperName);
    Serial.print("] ");

    if (event.added)
    {
        Serial.print("OBJECT ADDED: ");
    }
    else
    {
        Serial.print("OBJECT REMOVED: ");
    }

    Serial.println(event.uid);
}
