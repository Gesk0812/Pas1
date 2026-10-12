---
name: JUCE UTF-8 UI text
description: JUCE narrow-string semantics and the project's explicit UTF-8 conversion rule.
---

JUCE's `String(const char*)` interprets a plain narrow C string as ASCII, even when the source file is UTF-8 and MSVC compiles it with `/utf-8`. Russian UI text therefore needs explicit decoding through `juce::String::fromUTF8` before being passed to JUCE.

**Why:** The original Windows UI displayed mojibake despite valid UTF-8 bytes and a successful build; the root cause was JUCE's ASCII interpretation, not the editor locale or a stale plugin binary.

**How to apply:** Use `pas1::ui::utf8String(...)` for non-ASCII literals passed to JUCE APIs, and `utf8StringArray(...)` for choice lists. When concatenating localized strings, decode each non-ASCII piece before concatenation. Do not alter UTF-8 literals used by standard-library text parsers.
