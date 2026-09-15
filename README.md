# ANOPI

<img width="1082" height="708" alt="image" src="https://github.com/user-attachments/assets/78027ce8-4ba3-41d1-87a3-0aa57106cc06" />

> Open chord-builder instrument inspired by the NOPIA Mk1 harmony workflow

## Play it

Build the **Standalone** app (or load the AU/VST3) and click the panel so it has keyboard focus.

- **A W S E D F T G Y H U J K** — Chord Builder (A is always degree I in Static mode)
- **1–9 0 - =** — Tonal Selector (C through B)
- **← → ↑ ↓** — SCALE grid (left darker, right brighter, up/down parallel family)
- **[ ]** — extensions (5 → 3 → 7 → 9 → 11 → 13)
- **Shift** — second chord (sus / dominant / tritone-sub)
- **M** — major/minor &nbsp; **Tab** — Static / Real scale
- **Space** — sustain &nbsp; **; '** — voicing &nbsp; **Z X C V** — strum chord tones
- **B / N** — bass root / alternate &nbsp; **F1–F4** — Keys / Bass / Arp / Pad
- MIDI: play octave (default C3–C4) is the Chord Builder; C5–B5 is the Tonal Selector; CC 16/17 extensions/voicing; CC 64 sustain

**Internal tones** are on by default so Standalone is immediately musical (Keys / Bass / Arp / Pad). Turn them off when you want ANOPI to drive other instruments only.

## Four parts in a DAW

ANOPI emits MIDI on four channels from one resolve:

| Module | Channel |
| --- | --- |
| Keys | 1 |
| Bass | 2 |
| Arp | 3 (off by default) |
| Pad | 4 |

**Live record (Reaper / Live / Bitwig):** put ANOPI on a MIDI/instrument track. Create four more MIDI tracks whose input is ANOPI, each filtered to channel 1–4, each with its own synth. Arm them and play. Parts stay in harmonic lock.

**Standalone virtual cables:** set Output Mode to *Virtual cables* to create ports `ANOPI Keys/Bass/Arp/Pad` (macOS; Windows needs a loopback port).

**Dump:** enable Capture, play a take, then **Dump MIDI** for a Type 1 SMF with four tracks.

Logic’s plugin MIDI-out is awkward — use Standalone cables or IAC.

## Build

Needs CMake 3.22+, a C++20 compiler, and git (JUCE 8.0.8 is fetched on first configure).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
cmake --build build --target anopi_tests
ctest --test-dir build --output-on-failure
```

Targets: `ANOPI_Standalone`, `ANOPI_VST3`, and on macOS `ANOPI_AU`. With `COPY_PLUGIN_AFTER_BUILD` the plugins also land in the OS plugin folders.

JUCE is licensed AGPLv3 unless you have a commercial JUCE license — this project inherits that.

## Releases

Publishing a GitHub Release triggers [`.github/workflows/release.yml`](.github/workflows/release.yml): macOS AU+VST3+Standalone, Windows VST3, Linux VST3, then upload as release assets.

## Disclaimer

ANOPI is an independent development inspired by public descriptions. Not affiliated with any other vendors.
