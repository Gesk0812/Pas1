---
name: JUCE Linux build prerequisites
description: Local JUCE plugin builds in the current Replit Linux container are blocked by missing X11 development headers.
---

The current development container cannot configure the full JUCE plugin target because `juceaide` fails to compile without `X11/Xlib.h`. The non-GUI DSP/core test target can still be configured and tested successfully.

**Why:** A local configure attempt failed at JUCE's GUI helper build before compiling the plugin sources, so a green core test does not confirm the VST3/GUI target compiles.

**How to apply:** Use GitHub Actions or another Linux environment with JUCE's X11 development prerequisites for full plugin verification; report the scope of local core tests separately.
