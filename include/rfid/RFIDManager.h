#pragma once

#include <Arduino.h>

struct RFIDEvent
{
    String uid;
    bool added;
};

class RFIDManager
{
public:
    explicit RFIDManager(HardwareSerial &serial);

    void begin();
    void update();
    bool getEvent(RFIDEvent &event);

private:
    static const int MAX_TAGS = 50;
    static const unsigned long REMOVE_TIMEOUT = 3000;

    struct Tag
    {
        String uid;
        unsigned long lastSeen;
        bool present;
    };

    HardwareSerial &serial;
    String inputLine;

    Tag tags[MAX_TAGS];
    int tagCount = 0;

    RFIDEvent events[MAX_TAGS * 2];
    int eventRead = 0;
    int eventWrite = 0;

    void processLine(String line);
    void addEvent(const String &uid, bool added);
};