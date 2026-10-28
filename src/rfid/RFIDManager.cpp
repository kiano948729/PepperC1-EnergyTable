#include "rfid/RFIDManager.h"

RFIDManager::RFIDManager(HardwareSerial &serial)
    : serial(serial) {}

void RFIDManager::begin()
{
    inputLine.reserve(320);
}

void RFIDManager::update()
{
    while (serial.available())
    {
        char c = serial.read();

        if (c == '\n' || c == '\r')
        {
            if (inputLine.length() > 0)
            {
                processLine(inputLine);
                inputLine = "";
            }
        }
        else
        {
            inputLine += c;

            if (inputLine.length() > 300)
            {
                inputLine = "";
            }
        }
    }

    unsigned long now = millis();

    for (int i = 0; i < tagCount; i++)
    {
        if (tags[i].present &&
            now - tags[i].lastSeen >= REMOVE_TIMEOUT)
        {
            tags[i].present = false;
            addEvent(tags[i].uid, false);
        }
    }
}

void RFIDManager::processLine(String line)
{
    int pos = line.indexOf("UID: ");
    if (pos < 0)
        return;

    String uid = line.substring(pos + 5);
    int end = uid.indexOf(',');

    if (end >= 0)
    {
        uid = uid.substring(0, end);
    }

    uid.trim();
    if (uid.isEmpty())
        return;

    unsigned long now = millis();

    for (int i = 0; i < tagCount; i++)
    {
        if (tags[i].uid == uid)
        {
            tags[i].lastSeen = now;

            if (!tags[i].present)
            {
                tags[i].present = true;
                addEvent(uid, true);
            }

            return;
        }
    }

    if (tagCount >= MAX_TAGS)
        return;

    tags[tagCount] = {uid, now, true};
    tagCount++;

    addEvent(uid, true);
}

void RFIDManager::addEvent(const String &uid, bool added)
{
    int next = (eventWrite + 1) % (MAX_TAGS * 2);

    if (next == eventRead)
        return;

    events[eventWrite] = {uid, added};
    eventWrite = next;
}

bool RFIDManager::getEvent(RFIDEvent &event)
{
    if (eventRead == eventWrite)
        return false;

    event = events[eventRead];
    eventRead = (eventRead + 1) % (MAX_TAGS * 2);

    return true;
}