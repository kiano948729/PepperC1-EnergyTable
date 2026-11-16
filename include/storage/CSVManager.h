#pragma once

#include <Arduino.h>

struct TagData
{
    String uid;
    String consumption;
    String generation;
};

class CSVManager
{
public:
    bool begin();

    bool isReady() const;

    bool ensureTag(
        const String &uid,
        bool &wasAdded
    );

    bool tagExists(const String &uid);

    bool getTagData(
        const String &uid,
        TagData &data
    );

    bool updateTagData(
        const String &uid,
        const String &consumption,
        const String &generation
    );

    // Stuurt alle opgeslagen regels naar de computer.
    bool sendAll(Stream &stream);

private:
    const char *FILE_PATH = "/tags.csv";

    bool mounted = false;

    String normalizeUID(const String &uid);

    bool createFileIfMissing();

    bool parseRow(
        const String &line,
        TagData &data
    );
};