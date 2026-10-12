#pragma once

#include <juce_core/juce_core.h>

#include <initializer_list>

namespace pas1::ui {

// JUCE treats a plain const char* as ASCII. Decode UTF-8 UI literals explicitly.
inline juce::String utf8String(const char* text)
{
    return juce::String::fromUTF8(text);
}

inline juce::StringArray utf8StringArray(std::initializer_list<const char*> values)
{
    juce::StringArray result;
    for (const auto* value : values)
        result.add(utf8String(value));
    return result;
}

} // namespace pas1::ui
