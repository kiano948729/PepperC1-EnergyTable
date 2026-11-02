#pragma once

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <cstdint>
#include <vector>

// The NeoPixelBus type used to drive the strip. Override it BEFORE including
// this header (or via build flags) if you use a different color order or
// transport, e.g.:
//   #define SECTIONCOMETS_STRIP_TYPE NeoPixelBus<NeoRgbFeature, NeoEsp32I2s1Ws2812xMethod>
#ifndef SECTIONCOMETS_STRIP_TYPE
#define SECTIONCOMETS_STRIP_TYPE NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0800KbpsMethod>
#endif

struct SectionColor
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct CometOptions
{
    SectionColor color{80, 80, 80};
    int direction = 1;
    float speed = 20.0f;
    uint16_t tailLength = 3;
    uint16_t gap = 1;
    float position = 0.0f;
    bool showHead = true;
    bool paused = false;
};

struct Comet
{
    int section;
    SectionColor color;
    int direction; // +1 or -1
    float speed;   // logical LEDs per second
    uint16_t tailLength;
    uint16_t gap;   // dark LEDs between head and tail
    float position; // current head position within the section
    bool showHead;
    bool paused;
};

class SectionComets
{
public:
    using Section = std::vector<int>;
    using StripType = SECTIONCOMETS_STRIP_TYPE;

    // pin: data pin, numLeds: total LEDs on the strip,
    // maxBrightness: hard cap per color channel (0-255) to protect your PSU.
    SectionComets(uint8_t pin, uint16_t numLeds, uint8_t maxBrightness = 80);
    ~SectionComets();
    SectionComets(const SectionComets &) = delete;
    SectionComets &operator=(const SectionComets &) = delete;

    // Call once in setup().
    void begin();

    // ---- Sections -----------------------------------------------------------
    // Replace all sections. Invalid LED indices (out of range) are dropped.
    void setSections(const std::vector<Section> &sections);
    // Split the strip into equal sections: {0..n-1}, {n..2n-1}, ...
    void setUniformSections(uint16_t sectionSize);
    // Add one section from an explicit list of LED indices (any order, so
    // reversed or zig-zag sections work). Returns its index, or -1 if empty.
    int addSection(const Section &leds);
    // Add one section covering LEDs first..last inclusive (first > last
    // gives a reversed section). Returns its index, or -1 if invalid.
    int addSectionRange(int first, int last);
    void clearSections(); // also removes all comets
    size_t sectionCount() const { return sections_.size(); }
    size_t sectionLength(int section) const;

    // ---- Comets -------------------------------------------------------------
    // Returns the comet's index, or -1 if the section is invalid.
    int addComet(int section, const CometOptions &options = {});
    // Positional overload retained for existing sketches.
    int addComet(int section, SectionColor color, int direction = 1,
                 float speed = 20.0f, uint16_t tailLength = 3, uint16_t gap = 1,
                 float startPosition = 0.0f, bool showHead = true);
    void setCometPosition(int comet, float position);
    void setCometSpeed(int comet, float speed);
    void setCometColor(int comet, SectionColor color);
    void pauseComet(int comet);
    void resumeComet(int comet);
    void clearSectionComets(int section);
    void clearAllComets();
    std::vector<Comet> &comets() { return comets_; }

    // ---- Rendering ----------------------------------------------------------
    // One full frame: clear, advance comets, draw, push to the strip.
    // Set moveComets to false to render the current positions without moving.
    void update(bool moveComets = true);
    // Individual steps, for mixing with your own effects:
    void clear();
    void advance(uint32_t now);
    void render();
    void show();

    // ---- Drawing helpers ----------------------------------------------------
    // Position wraps within the section. Brightness is capped by maxBrightness.
    void setPixel(int coordinate, int section, SectionColor color);
    void addPixel(int coordinate, int section, SectionColor color); // blends
    void blink(SectionColor color, uint32_t periodMs = 1000);
    void colorBlink(SectionColor a, SectionColor b, uint32_t periodMs = 1000);

    void setMaxBrightness(uint8_t value) { maxBrightness_ = value; }
    uint8_t maxBrightness() const { return maxBrightness_; }
    uint16_t numLeds() const { return numLeds_; }

    // Direct access to the underlying NeoPixelBus object.
    StripType &strip() { return *strip_; }

private:
    bool ready() const { return strip_ != nullptr; }
    bool validSection(int section) const;
    uint8_t cap(int value) const;

    uint8_t pin_;
    uint16_t numLeds_;
    uint8_t maxBrightness_;
    StripType *strip_;
    std::vector<Section> sections_;
    std::vector<Comet> comets_;
    uint32_t lastUpdate_ = 0;
};