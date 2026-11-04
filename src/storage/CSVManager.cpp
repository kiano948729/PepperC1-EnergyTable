#include "storage/CSVManager.h"
#include <LittleFS.h>

String CSVManager::normalizeUID(const String &uid)
{
    String result = uid;
    result.trim();
    result.toUpperCase();

    return result;
}

bool CSVManager::begin()
{
    mounted = false;

    // Bestaande gegevens niet automatisch formatteren.
    if (!LittleFS.begin(false))
    {
        Serial.println("LITTLEFS: MOUNT FAILED");
        return false;
    }

    mounted = true;

    if (!createFileIfMissing())
    {
        Serial.println("LITTLEFS: CSV CREATION FAILED");

        LittleFS.end();
        mounted = false;

        return false;
    }

    return true;
}

bool CSVManager::isReady() const
{
    return mounted;
}

bool CSVManager::createFileIfMissing()
{
    if (!mounted)
        return false;

    if (LittleFS.exists(FILE_PATH))
        return true;

    File file = LittleFS.open(FILE_PATH, FILE_WRITE);

    if (!file)
        return false;

    file.println("UID,Consumption,Generation");
    file.close();

    return true;
}

bool CSVManager::parseRow(
    const String &line,
    TagData &data
)
{
    if (line.length() == 0 || line.startsWith("UID,"))
        return false;

    int firstComma = line.indexOf(',');

    if (firstComma < 0)
        return false;

    int secondComma = line.indexOf(',', firstComma + 1);

    if (secondComma < 0)
        return false;

    data.uid = normalizeUID(
        line.substring(0, firstComma)
    );

    data.consumption =
        line.substring(firstComma + 1, secondComma);

    data.generation =
        line.substring(secondComma + 1);

    data.consumption.trim();
    data.generation.trim();

    return data.uid.length() > 0;
}

bool CSVManager::tagExists(const String &uid)
{
    if (!mounted)
        return false;

    String wantedUID = normalizeUID(uid);

    if (wantedUID.length() == 0 ||
        !LittleFS.exists(FILE_PATH))
        return false;

    File file = LittleFS.open(FILE_PATH, FILE_READ);

    if (!file)
        return false;

    bool found = false;

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        TagData data;

        if (parseRow(line, data) &&
            data.uid == wantedUID)
        {
            found = true;
            break;
        }
    }

    file.close();

    return found;
}

bool CSVManager::getTagData(
    const String &uid,
    TagData &data
)
{
    if (!mounted)
        return false;

    String wantedUID = normalizeUID(uid);

    if (wantedUID.length() == 0 ||
        !LittleFS.exists(FILE_PATH))
        return false;

    File file = LittleFS.open(FILE_PATH, FILE_READ);

    if (!file)
        return false;

    bool found = false;

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        TagData current;

        if (parseRow(line, current) &&
            current.uid == wantedUID)
        {
            data = current;
            found = true;
            break;
        }
    }

    file.close();

    return found;
}

bool CSVManager::ensureTag(
    const String &uid,
    bool &wasAdded
)
{
    wasAdded = false;

    if (!mounted)
        return false;

    String cleanUID = normalizeUID(uid);

    if (cleanUID.length() == 0 ||
        cleanUID.indexOf(',') >= 0 ||
        cleanUID.indexOf('\n') >= 0 ||
        cleanUID.indexOf('\r') >= 0)
        return false;

    if (tagExists(cleanUID))
        return true;

    File file = LittleFS.open(FILE_PATH, FILE_APPEND);

    if (!file)
        return false;

    size_t written = file.println(cleanUID + ",,");

    file.close();

    if (written == 0)
        return false;

    wasAdded = true;

    return true;
}

bool CSVManager::updateTagData(
    const String &uid,
    const String &consumption,
    const String &generation
)
{
    if (!mounted)
        return false;

    String wantedUID = normalizeUID(uid);

    if (wantedUID.length() == 0 ||
        consumption.indexOf(',') >= 0 ||
        generation.indexOf(',') >= 0 ||
        consumption.indexOf('\n') >= 0 ||
        generation.indexOf('\n') >= 0)
        return false;

    if (!tagExists(wantedUID))
        return false;

    const char *TEMP_PATH = "/tags.tmp";

    File input = LittleFS.open(FILE_PATH, FILE_READ);

    if (!input)
        return false;

    File output = LittleFS.open(TEMP_PATH, FILE_WRITE);

    if (!output)
    {
        input.close();
        return false;
    }

    bool updated = false;

    while (input.available())
    {
        String line = input.readStringUntil('\n');
        line.trim();

        TagData current;

        if (parseRow(line, current) &&
            current.uid == wantedUID)
        {
            line = wantedUID + "," +
                   consumption + "," +
                   generation;

            updated = true;
        }

        output.println(line);
    }

    input.close();
    output.close();

    if (!updated)
    {
        LittleFS.remove(TEMP_PATH);
        return false;
    }

    LittleFS.remove(FILE_PATH);

    if (!LittleFS.rename(TEMP_PATH, FILE_PATH))
        return false;

    return true;
}

bool CSVManager::sendAll(Stream &stream)
{
    if (!mounted || !LittleFS.exists(FILE_PATH))
        return false;

    File file = LittleFS.open(FILE_PATH, FILE_READ);

    if (!file)
        return false;

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        TagData data;

        if (!parseRow(line, data))
            continue;

        stream.print("TAG\t");
        stream.print(data.uid);
        stream.print('\t');
        stream.print(data.consumption);
        stream.print('\t');
        stream.println(data.generation);
    }

    file.close();

    return true;
}