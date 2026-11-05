# Peaky
**Automatic pre-compression vocal peak smoother** (VST3 / AU / Standalone) — developed by **[Aron H](https://www.instagram.com/aronh.official)**.

Peaky does what an engineer does with clip-gain before an 1176: it finds the loud peaks and sudden transients in a vocal and
pulls them down *before* the compressor sees them — with a smooth, lookahead-timed gain ramp instead of attack/release behaviour,
so there is no pumping and no distortion.

## Signal flow
```
input ─┬─► detector (RMS or Peak, stereo-linked) ─► gain computer ─► sliding-min (lookahead) ─► 2× moving-average ─► gain (dB→lin)
       └─► delay line (= lookahead) ───────────────────────────────────────────────────────────► × gain × post-gain ─► mix ─► output
```
1. **Detector** – on the un-delayed signal. *PEAK* = max |sample| across channels. *RMS* = 3 ms running RMS scaled by +3 dB so both modes read on the same scale.
2. **Gain computer** – above `Threshold` a 4 dB soft knee engages; the plugin then removes 80 % of the excess above `Target Level`, never more than `Max Gain Red`.
   (If Target is higher than Threshold, smoothing starts where the level crosses Target.)
3. **Lookahead smoothing** – a sliding-window minimum over `lookahead+1` samples makes sure the attenuation is already in place when the peak
   arrives; two cascaded moving averages turn that into a triangular, click-free ramp. There is no release stage, the same ramp comes back up after the peak.
   *The ramp length = `Smoothing`, limited to the lookahead time* (pick a longer lookahead for longer smoothing; the factory presets already do).
4. **Delay + apply** – audio is delayed by the lookahead time (plugin latency is reported to the host), gain is applied, then post-gain and dry/wet mix.
   Dry is delayed too, so the mix stays phase-aligned.
5. **Lookahead OFF** – zero-latency mode: 0.5 ms attack, `Smoothing` as release. Less transparent; meant for live monitoring.

## Controls
| Control | Range | Default |
|---|---|---|
| Threshold | −48 … 0 dB | −10 dB |
| Target Level | −36 … −3 dB | −12 dB |
| Smoothing | 5 … 50 ms | 15 ms |
| Max Gain Red | 0 … −18 dB | −8 dB |
| Post Gain | −12 … +12 dB | +2 dB |
| Lookahead | ON/OFF + 5/10/15/20/30/50 ms (click the time to cycle) | ON, 15 ms |
| Detect Mode | RMS / PEAK | RMS |
| Mix | 0 … 100 % | 100 % |
| Oversample | 2× on/off (halfband FIR; more accurate peak detection, adds latency, reported to host) | off |
| Bypass | LED lit = processing, click to bypass (also follows the host bypass) | active |

## Presets
* 20 factory presets (`Presets/Factory/*.peakypreset`, also compiled into the plugin).
* **Save / load your own:** gear icon → *Save Preset…* (stored in `Documents/Aron H/Peaky/Presets`), *Export / Load Preset from File…*, *Delete*, *Open folder*.
* Edit the factory list in `tools/gen_presets.py`, run it, rebuild.

## Build
Requires CMake ≥ 3.22 and a C++17 compiler (Xcode / Visual Studio 2022). JUCE 8 is fetched automatically.
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Installers
* **Windows** – `installer/windows/Peaky.iss` (Inno Setup) → installs to `C:\Program Files\Common Files\VST3\Peaky.vst3`.
* **macOS** – `bash installer/mac/build_pkg.sh` → `.pkg` (VST3 → `/Library/Audio/Plug-Ins/VST3`, AU → `/Library/Audio/Plug-Ins/Components`) plus `Peaky-macOS-plugins.zip`.
  Plugins are ad-hoc signed and the quarantine flag is stripped on install.
* **GitHub Actions** (`.github/workflows/build.yml`) builds both on every push; push a tag like `v1.0.0` and the installers are attached to a GitHub Release.
  Optional: add a `DEV_ID_INSTALLER` secret to sign the macOS package (notarisation is still needed for a warning-free install on other Macs).

## Installing on macOS without Gatekeeper warnings
The plugins are not notarized (that needs a paid Apple Developer ID), so a `.pkg` downloaded in a browser is quarantined by macOS. Use one of these instead:

**1. One-line install (recommended)** – curl downloads are never quarantined, and the script also runs `xattr -dr com.apple.quarantine`:
```bash
curl -fsSL https://raw.githubusercontent.com/YOUR_GITHUB_USER/Peaky/main/install-mac.sh | bash
# for all users:  ... | bash -s -- --system
```
(Replace `YOUR_GITHUB_USER` in `install-mac.sh` and here with your GitHub name, or set `PEAKY_REPO=user/Peaky`.)

**2. Homebrew** – create a repo named `homebrew-peaky`, run `installer/mac/make_cask.sh 1.0.0 YOUR_GITHUB_USER Peaky-macOS-plugins.zip` and commit `Casks/peaky.rb`. Users then run:
```bash
brew tap YOUR_GITHUB_USER/peaky
brew install --cask peaky
```

**3. Already downloaded the .pkg / .zip?**
```bash
xattr -dr com.apple.quarantine ~/Downloads/Peaky-*.pkg
```
or right-click the pkg → Open, or System Settings → Privacy & Security → *Open Anyway*. If a DAW still blocks an installed plugin:
`xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/Peaky.vst3`

To remove warnings entirely, sign with a Developer ID and notarize (secrets `DEV_ID_APPLICATION`, `DEV_ID_INSTALLER`).

## License
JUCE and the VST3 SDK are dual-licensed (GPLv3 / commercial). For a public GitHub repo, release Peaky under **GPLv3** (add a `LICENSE` file),
or buy a JUCE commercial license if you want to distribute closed-source.

© Aron H — https://www.instagram.com/aronh.official
