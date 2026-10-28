#include "leds/LedManager.h"

#include <algorithm>
#include <cmath>

namespace
{
    int wrapCoordinate(int coordinate, int length)
    {
        coordinate %= length;
        return coordinate < 0 ? coordinate + length : coordinate;
    }

    uint8_t clampChannel(int value)
    {
        return static_cast<uint8_t>(std::max(0, std::min(255, value)));
    }

    SectionColor scaleColor(SectionColor c, int numerator, int denominator)
    {
        if (denominator <= 0)
        {
            return {0, 0, 0};
        }
        return {clampChannel(c.red * numerator / denominator),
                clampChannel(c.green * numerator / denominator),
                clampChannel(c.blue * numerator / denominator)};
    }
} // namespace

SectionComets::SectionComets(uint8_t pin, uint16_t numLeds, uint8_t maxBrightness)
    : pin_(pin), numLeds_(numLeds), maxBrightness_(maxBrightness), strip_(nullptr)
{
}

SectionComets::~SectionComets()
{
    delete strip_;
}

void SectionComets::begin()
{
    if (!strip_)
    {
        strip_ = new StripType(numLeds_, pin_);
    }
    strip_->Begin();
    clear();
    strip_->Show();
    lastUpdate_ = millis();
}

// ---- Sections ---------------------------------------------------------------

void SectionComets::setSections(const std::vector<Section> &sections)
{
    clearSections();
    for (const Section &s : sections)
    {
        addSection(s);
    }
}

void SectionComets::setUniformSections(uint16_t sectionSize)
{
    clearSections();
    if (sectionSize == 0)
    {
        return;
    }
    for (int start = 0; start < numLeds_; start += sectionSize)
    {
        addSectionRange(start, std::min<int>(start + sectionSize, numLeds_) - 1);
    }
}

int SectionComets::addSection(const Section &leds)
{
    Section valid;
    for (int led : leds)
    {
        if (led >= 0 && led < numLeds_)
        {
            valid.push_back(led);
        }
    }
    if (valid.empty())
    {
        return -1;
    }
    sections_.push_back(valid);
    return static_cast<int>(sections_.size()) - 1;
}

int SectionComets::addSectionRange(int first, int last)
{
    Section leds;
    const int step = first <= last ? 1 : -1;
    for (int i = first; i != last + step; i += step)
    {
        leds.push_back(i);
    }
    return addSection(leds);
}

void SectionComets::clearSections()
{
    comets_.clear();
    sections_.clear();
}

size_t SectionComets::sectionLength(int section) const
{
    return validSection(section) ? sections_[section].size() : 0;
}

bool SectionComets::validSection(int section) const
{
    return section >= 0 && section < static_cast<int>(sections_.size()) &&
           !sections_[section].empty();
}

uint8_t SectionComets::cap(int value) const
{
    return static_cast<uint8_t>(std::max(0, std::min<int>(value, maxBrightness_)));
}

// ---- Comets -----------------------------------------------------------------

int SectionComets::addComet(int section, const CometOptions &options)
{
    if (!validSection(section))
    {
        return -1;
    }
    Comet comet;
    comet.section = section;
    comet.color = options.color;
    comet.direction = options.direction < 0 ? -1 : 1;
    comet.speed = options.speed;
    comet.tailLength = options.tailLength;
    comet.gap = options.gap;
    comet.position = options.position;
    comet.showHead = options.showHead;
    comet.paused = options.paused;
    comets_.push_back(comet);
    return static_cast<int>(comets_.size()) - 1;
}

int SectionComets::addComet(int section, SectionColor color, int direction,
                            float speed, uint16_t tailLength, uint16_t gap,
                            float startPosition, bool showHead)
{
    CometOptions options;
    options.color = color;
    options.direction = direction;
    options.speed = speed;
    options.tailLength = tailLength;
    options.gap = gap;
    options.position = startPosition;
    options.showHead = showHead;
    return addComet(section, options);
}

void SectionComets::setCometPosition(int comet, float position)
{
    if (comet >= 0 && comet < static_cast<int>(comets_.size()))
    {
        comets_[comet].position = position;
    }
}

void SectionComets::setCometSpeed(int comet, float speed)
{
    if (comet >= 0 && comet < static_cast<int>(comets_.size()))
    {
        comets_[comet].speed = speed;
    }
}

void SectionComets::setCometColor(int comet, SectionColor color)
{
    if (comet >= 0 && comet < static_cast<int>(comets_.size()))
    {
        comets_[comet].color = color;
    }
}

void SectionComets::pauseComet(int comet)
{
    if (comet >= 0 && comet < static_cast<int>(comets_.size()))
    {
        comets_[comet].paused = true;
    }
}

void SectionComets::resumeComet(int comet)
{
    if (comet >= 0 && comet < static_cast<int>(comets_.size()))
    {
        comets_[comet].paused = false;
    }
}

void SectionComets::clearSectionComets(int section)
{
    comets_.erase(std::remove_if(comets_.begin(), comets_.end(),
                                 [section](const Comet &c)
                                 { return c.section == section; }),
                  comets_.end());
}

void SectionComets::clearAllComets()
{
    comets_.clear();
}

// ---- Rendering --------------------------------------------------------------

void SectionComets::update(bool moveComets)
{
    if (!ready())
    {
        return;
    }
    clear();
    if (moveComets)
    {
        advance(millis());
    }
    else
    {
        lastUpdate_ = millis();
    }
    render();
    show();
}

void SectionComets::clear()
{
    if (!ready())
    {
        return;
    }
    strip_->ClearTo(RgbColor(0, 0, 0));
}

void SectionComets::advance(uint32_t now)
{
    if (!ready())
    {
        return;
    }
    const uint32_t elapsed = now - lastUpdate_;
    lastUpdate_ = now;

    for (Comet &comet : comets_)
    {
        if (comet.paused)
        {
            continue;
        }
        const int length = sections_[comet.section].size();
        if (length == 0)
        {
            continue;
        }
        comet.position = std::fmod(
            comet.position + comet.direction * comet.speed * (elapsed / 1000.0f),
            static_cast<float>(length));
        if (comet.position < 0)
        {
            comet.position += length;
        }
    }
}

void SectionComets::render()
{
    for (const Comet &comet : comets_)
    {
        const int head = static_cast<int>(std::floor(comet.position));
        if (comet.showHead)
        {
            addPixel(head, comet.section, comet.color);
        }

        for (uint16_t offset = 1; offset <= comet.tailLength; offset++)
        {
            const int coordinate =
                head - comet.direction * (static_cast<int>(comet.gap) + static_cast<int>(offset) - 1);
            const int remaining = comet.tailLength - offset + 1;
            addPixel(coordinate, comet.section,
                     scaleColor(comet.color, remaining, comet.tailLength));
        }
    }
}

// ---- Drawing helpers --------------------------------------------------------

void SectionComets::setPixel(int coordinate, int section, SectionColor color)
{
    if (!ready() || !validSection(section))
    {
        return;
    }
    coordinate = wrapCoordinate(coordinate, sections_[section].size());
    strip_->SetPixelColor(sections_[section][coordinate],
                          RgbColor(cap(color.red), cap(color.green), cap(color.blue)));
}

void SectionComets::addPixel(int coordinate, int section, SectionColor color)
{
    if (!ready() || !validSection(section))
    {
        return;
    }
    coordinate = wrapCoordinate(coordinate, sections_[section].size());
    const int pixel = sections_[section][coordinate];
    const RgbColor existing = strip_->GetPixelColor(pixel);
    strip_->SetPixelColor(pixel, RgbColor(cap(existing.R + color.red),
                                          cap(existing.G + color.green),
                                          cap(existing.B + color.blue)));
}

void SectionComets::blink(SectionColor color, uint32_t periodMs)
{
    if (!ready() || periodMs == 0)
    {
        return;
    }
    const bool on = (millis() % periodMs) < periodMs / 2;
    strip_->ClearTo(on ? RgbColor(cap(color.red), cap(color.green), cap(color.blue))
                       : RgbColor(0, 0, 0));
}

void SectionComets::colorBlink(SectionColor a, SectionColor b, uint32_t periodMs)
{
    if (!ready() || periodMs == 0)
    {
        return;
    }
    const bool on = (millis() % periodMs) < periodMs / 2;
    const SectionColor c = on ? a : b;
    strip_->ClearTo(RgbColor(cap(c.red), cap(c.green), cap(c.blue)));
}

void SectionComets::show()
{
    if (ready())
    {
        strip_->Show();
    }
}