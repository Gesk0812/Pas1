# PAS 1 — Synth Engine in progress

PAS 1 is a native C++17 synthesizer project built with JUCE and CMake. Its current implementation extends the initial polyphonic MIDI instrument with a multi-source wavetable engine, a per-voice modulation matrix, filtering, and a stereo effects chain.

## What is included

- VST3 and Standalone plugin targets.
- 32 preallocated voices; the Voices parameter sets the active limit from 1 to 32.
- Poly, Mono, and last-note-priority Legato modes; monophonic note release restores the most recently held key.
- Optional sample-accurate portamento with an adjustable 1 ms–2 s duration, including retargeting during an active glide.
- MIDI note-on/off, velocity, pitch bend, CC1 modulation, sustain/sostenuto pedals, and aftertouch.
- Attack, Decay, Sustain, Release, waveform, voice count, and master output parameters.
- Host automation and JUCE AudioProcessorValueTreeState state save/restore.
- A DSP test executable that does not require JUCE or GoogleTest.
- GitHub Actions workflow configured for Windows, macOS, and Linux builds.
- A mipmapped, interpolated wavetable oscillator with sine, triangle, saw, and square tables.
- Oscillator 1 wavetable scan, octave/fine tuning, phase, and up to seven unison voices.
- Oscillator 2 wavetable, detuning, phase offset, hard sync, FM, and ring modulation.
- Oscillator 3 selectable waveform and pulse width; sub oscillator and white-noise source.
- A six-mode metallic music-box tine engine with adjustable mix, brightness, decay, resonance, inharmonicity, and strike position.
- Four additional algorithmic ancient-inspired voices: double-pipe aulos, plucked lyre, modal bronze gong, and irregular found-metal resonances, with mix, brightness, decay, air/strike, and pipe-detune controls.
- A per-voice state-variable filter with low-pass, high-pass, band-pass, and notch modes.
- Filter drive/saturation, a separate filter ADSR, key tracking, velocity, mod-wheel and random-per-note cutoff modulation.
- Four per-voice LFOs with sine, triangle, saw, square, sample-and-hold, and smooth-random shapes.
- Free and host-tempo-synced LFO rates, phase/retrigger/fade controls, and routing to pitch, filter cutoff, or oscillator 2 level.
- A 16-route modulation matrix with LFOs 1–4, both envelopes, velocity, mod wheel, pitch bend, key tracking, and per-note random sources; targets include pitch, oscillator level, wavetable position, filter, FM, pan, effect mix, and Morph X/Y.
- Per-route source, destination, amount, polarity, and enable controls in a scrollable native UI; routes are host-automatable and saved with the instrument state.
- Distortion, chorus, phaser, delay, reverb, broad two-band EQ, and a soft output limiter; delay and reverb buffers are allocated during preparation, not in the audio callback.
- A dedicated preset library with search, category/favorite/recent filters, name/category/recent sorting, 70 built-in sounds, and versioned `.pas1preset` import/export.
- User preset Save, Save As, duplicate, rename, delete, and favorite/recent metadata stored separately from the DAW project state.
- An “ALL PARAMETERS” editor page exposing every automatable parameter.
- A local Sound Recipe generator with nine character controls, keyword mapping, ten variation modes, deterministic seeds, parameter and section locks, 4/16-variation sets, before/after preview, undo/redo, keep/revert, and `.pas1recipe` save/load.
- A four-corner Morph Matrix with factory and user-captured A/B/C/D sounds, seeded corner randomization, per-parameter locks, bilinear interpolation, logarithmic cutoff/time mapping, and smoothed transitions.
- Host-automatable Morph X/Y controls that continue changing the sound when the editor is closed; corner states and locks are included in saved plugin and user-preset state.
- Sound DNA profiles for brightness, harmonic complexity, density, movement, aggression, stereo width, bass weight, space, texture, and envelope character, calculated from actual synth parameters.
- DNA similarity ranking for the 16 built-in presets, preset comparison, profile blending, sound-space exploration, and generation from a blended or explored profile.
- A Sound DNA page alongside the core, Generator, Morph, and All Parameters views.
- Live MIDI activity and post-limiter output meters, with tooltips on the main controls.
- Dedicated Oscillators, Ancient Voices, Filters, Modulation, Morph, Generator, Sound DNA, Effects, Presets, and Settings tabs; the Ancient Voices page uses a separate rose/pink palette.
- A live, logarithmic output spectrum analyzer on the Home page, driven by a fixed-size atomic audio capture ring and computed on the UI thread.
- Settings-page factory reset with confirmation and full-state A/B snapshots for quick sound comparison.
- UI keyboard notes reach the synth through a fixed-capacity SPSC queue; the audio callback avoids JUCE's locking Synthesiser and MidiKeyboardState processing paths.
- DSP stress coverage across five sample rates (8–192 kHz) and five block sizes (1–1024 samples).

The remaining work is release validation: run the GitHub Actions build, install the VST3 in target DAWs, and verify host automation, keyboard input, state restore, preset import/export, and the interface in a real plugin host. Those checks remain intentionally deferred until the implementation stages are finished.

## Build locally

Install CMake 3.22 or newer, C and C++17 compilers, Git, and the platform's JUCE audio/UI build prerequisites. The CMake configure step downloads the pinned JUCE 8.0.8 source.

From the repository root:

```sh
cmake -S pas1 -B pas1/build -DCMAKE_BUILD_TYPE=Release
cmake --build pas1/build --config Release --parallel
ctest --test-dir pas1/build --build-config Release --output-on-failure
```

To run just the standalone DSP tests without fetching JUCE:

```sh
cmake -S pas1 -B pas1/build-tests -DPAS1_BUILD_PLUGIN=OFF -DPAS1_BUILD_TESTS=ON
cmake --build pas1/build-tests --config Release --parallel
ctest --test-dir pas1/build-tests --build-config Release --output-on-failure
```

Build output is under `pas1/build/PAS1_artefacts/Release/` (or `pas1/build/PAS1_artefacts/` for single-configuration generators). The VST3 folder is named `PAS 1.vst3`; the standalone application is alongside it.

## Build through GitHub

1. Create a GitHub repository and push the project root, including both `pas1/` and `.github/workflows/pas1-build.yml`.
2. Open **Actions** in the repository and run **PAS 1 build** (or push a change under `pas1/`).
3. Wait for the Windows, macOS, and Linux jobs to finish.
4. Download the `PAS1-Windows`, `PAS1-macOS`, or `PAS1-Linux` artifact from the completed workflow run.
5. On Windows, copy `PAS 1.vst3` into `%LOCALAPPDATA%\Programs\Common\VST3\` or your DAW's VST3 folder, then rescan plugins. Open `PAS 1.exe` to test it outside a DAW.
6. On macOS, copy the `.vst3` bundle to `~/Library/Audio/Plug-Ins/VST3/`, then rescan. The unsigned CI build may require local approval or signing before macOS will load it.
7. On Linux, copy the `.vst3` bundle to `~/.vst3/`, then rescan.

GitHub Actions builds and stores artifacts; it does not publish them as a GitHub Release.

## DAW smoke test

1. Load PAS 1 as a software instrument on a MIDI track.
2. Play MIDI notes or use the plugin's keyboard.
3. Confirm all four oscillator shapes produce audio.
4. Adjust Attack, Decay, Sustain, Release, Output, and Voices during playback.
5. Test note release and sustain pedal; move pitch bend and modulation wheel.
6. Save and reopen the DAW project and confirm the parameter values are restored.

The DSP tests cover oscillator signal/range and pulse width, wavetable scan, envelope stages, deterministic LFO output, filter stability/response, delay output, limiter behavior, stereo width, seeded recipe variations, morph interpolation, smoothing, locking, deterministic randomization, and Sound DNA profile analysis/blending. The host/DAW smoke test is still required to confirm compatibility with the target DAW.

## Ancient Voices

The ANCIENT page adds Aulos, Lyre, Bronze Gong, and Found Metal modes, plus six factory presets in the **Ancient** category. These are original additive/modal synthesis models inspired by public reporting about the instruments and metallic objects used in the score of *The Odyssey*. PAS 1 does not contain, sample, or reproduce the film's audio. The local recipe generator recognizes instrument names and can build a starting patch for each voice.

Background reading: [The Guardian](https://www.theguardian.com/film/2026/jul/23/the-odyssey-music-ludwig-goransson-christopher-nolan) and [Variety](https://variety.com/2026/artisans/news/the-odyssey-ludwig-goransson-film-score-1236813153/).

## Dependency and licensing notes

- JUCE is pinned to tag `8.0.8`. JUCE is dual-licensed under GPLv3 or a commercial JUCE license; review the current JUCE terms and choose the appropriate license before distributing a closed-source product.
- The VST3 implementation is provided by JUCE's included VST3 support. Review JUCE's distribution terms and Steinberg's current VST3 SDK licensing terms for the intended distribution model.
- The DSP core tests use no third-party test framework.
- GitHub-hosted runner images and their preinstalled toolchains are maintained by GitHub and are not version-pinned by this project.

## Roadmap

The implementation follows the supplied stages: core instrument, expanded synth engine, effects, presets, local rule-based sound generator, morph controls, Sound DNA, interface refinement, and release hardening. Implemented work includes the 16-route modulation matrix, tested mono note-priority behavior, and sample-accurate portamento. The preset-library portion includes search, filters, organization, and user-preset management. Dependency-free DSP/generator/DNA/modulation/portamento/note-priority tests are available. Remaining work is release validation: a fresh GitHub Actions run for the current source, VST3/DAW compatibility, host automation and state-restore checks, and interface testing in a real plugin host.
