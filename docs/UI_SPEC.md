# Peaky — UI design specification (660 × 460 px design size, shown at a fixed 120 % = 792 × 552 px, not resizable)

Use this as a prompt/spec for any designer or code generator. Brand: **PEAKY**, developed by **Aron H** (clickable link `https://www.instagram.com/aronh.official`).
Premium dark mode: charcoal brushed-metal body (`#25262B → #111215`, fine horizontal grain), electric teal `#35E0EE` for active states and value
read-outs, coral `#FF7F6E` for the reference lines and gain reduction, deep-blue `#2F5FD6` glowing knob rings, embossed silver PEAKY title.

## 1. Header (0–34 px)
* Left: small grey text "Developed by" (10 px) followed by the hyperlink **ARON H** (bold 15 px, opens the Instagram profile). No logo.
* Centre: **PEAKY** — bold 22 px, +12 % letter spacing, white.
* Right: `<` `>` preset step buttons, preset drop-down showing **"Natural Smoothing"** (dark rounded box, up/down chevrons), "..." menu button
  (Save Preset…, Export/Load Preset from File…, Delete User Preset, Open Presets Folder, About).

## 2. Visualizer (x 8, y 40, 566 × 220 px, rounded 6 px, near-black navy)
* Vertical scale at left: 0, −6, −12, −18, −24, −30, −36, −42, −48 dB (top → bottom, linear), faint horizontal grid lines.
* Scrolling ~4 s multi-layer waveform (translucent peak layer + smoothed inner layer, scan-line texture, bright teal edge), filled from the bottom with a gradient electric teal → blue → slate/charcoal.
* Coral gain-reduction curtain hanging from the top edge (depth = dB of reduction on the same scale).
* Two full-width backlit coral lines (soft glow + bright core): **PEAK THRESHOLD** (follows the Threshold knob) and **TARGET LEVEL** (follows the Target knob), each with a small
  bold caption (upper line caption above, lower line caption below).
* **Gain-reduction meter** (x 580, y 54, 72 × 206): caption "GAIN REDUCTION", 24 one-dB segments, blue (top two red), scale 0 / −4 / −8 / −12 / −16 / −20 / −24 dB at the left, white peak-hold triangle.

## 3. Knob row (y 266–400, five equal columns)
Each column: dark rounded value box above the knob (white bold 13 px), 84 px metal knob (knurled rim, white pointer) inside a blue value arc
(open at the bottom, 1.2π → 2.8π), min/max range labels at the lower-left/right of the knob, white bold caption below.

| # | Caption | Value box (default) | Range (left → right) |
|---|---|---|---|
| 1 | THRESHOLD | −10dB | −48dB → 0dB |
| 2 | TARGET LEVEL | −12dB | −36dB → −3dB |
| 3 | SMOOTHING | 15ms | 5ms → 50ms |
| 4 | MAX GAIN RED | −8dB | 0dB → −18dB |
| 5 | POST GAIN | +2dB | −12dB → +12dB |

Double-click resets to default; drag vertically/horizontally to change; click the value box to type.

## 4. Bottom global bar (y 404–460, darker strip)
| Group | Content |
|---|---|
| LOOKAHEAD | **ON** toggle (blue when on) + time button **10ms** style (click cycles 5/10/15/20/30/50) |
| DETECT MODE | **RMS** \| **PEAK** radio pair (selected = blue) |
| MIX | blue-filled horizontal slider with centred text **100%**, caption "DRY/WET" below |
| OVERSAMPLE | **2x** toggle (blue when on) |
| BYPASS | dark "ON" pill + green LED button (lit green = processing; click → amber "BYP" and dark-red LED) |

## 5. Behaviour
All controls are host-automatable parameters (IDs: threshold, target, smoothing, maxgr, postgain, lookahead, lookaheadtime, detect, mix, bypass).
Preset box lists 20 factory presets, then a "User" section. 30 fps repaint for scope and meter.
