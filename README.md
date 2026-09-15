# ANOPI

<img width="1082" height="708" alt="ANOPI chord builder" src="https://github.com/user-attachments/assets/78027ce8-4ba3-41d1-87a3-0aa57106cc06" />

> Open chord-builder instrument inspired by the NOPIA Mk1 harmony workflow

## Download

Get the latest Standalone app and plugins from the **[latest GitHub Release](https://github.com/lmangani/anopi/releases/latest)**.

Pick the build for your system: macOS (Standalone, AU, VST3, and an installer), Windows (Standalone and VST3), or Linux (Standalone and VST3).

## Play

Open the Standalone app (or load the AU/VST3 in a DAW) and click the panel so it has keyboard focus.

- **A W S E D F T G Y H U J K** — Chord Builder (A is always degree I in Static mode)
- **1–9 0 - =** — Tonal Selector (C through B)
- **← → ↑ ↓** — SCALE grid (left darker, right brighter, up/down parallel family)
- **[ ]** — extensions (5 → 3 → 7 → 9 → 11 → 13)
- **Shift** — second chord (sus / dominant / tritone-sub)
- **M** — major/minor &nbsp; **Tab** — Static / Real scale
- **Space** — sustain &nbsp; **; '** — voicing &nbsp; **Z X C V** — strum chord tones
- **B / N** — bass root / alternate &nbsp; **F1–F4** — Keys / Bass / Arp / Pad

**Internal tones** are on by default so Standalone is immediately musical (Keys / Bass / Arp / Pad). Turn them off when you want ANOPI to drive other instruments only.

**Capture** asks where to save a `.mid` (Keys / Bass / Arp / Pad tracks), then the button becomes **STOP**. **REC** (Standalone only) does the same for a `.wav` of the preview tones. **Save** / **Load** stash the current panel so you can reuse a setup.

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

Logic’s plugin MIDI-out is awkward — use Standalone cables or IAC.

## License

ANOPI is licensed under the [GNU Affero General Public License v3.0](LICENSE), matching the JUCE framework it is built with.

ANOPI is an independent development inspired by public descriptions. It is not affiliated with any other vendors.
