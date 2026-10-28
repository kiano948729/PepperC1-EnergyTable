#pragma once

#include <Arduino.h>
#include "RFIDManager.h"

class SerialHandler
{
public:
    void begin();
    void message(const String &text);
    void handleEvent(const RFIDEvent &event);
};