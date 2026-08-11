# 260728 Case Designs: Prototype (fits current modules) + Concept (ideal final)

Author: Donggyu An (Aiki)
Date: 2026-07-28
Basis: 260713 team progress/specs doc + 260718 GPS/LTE module update

Two versions in this package:

```text
A. PROTOTYPE (proto_*)   fits the modules we have RIGHT NOW, jumper wiring OK,
                         roomy, screw-open lid, print and use immediately
B. CONCEPT   (concept_*) the "later final version" target from the 260713 doc,
                         assumes an integrated PCB - design direction, not a fit
                         for current breakout boards
```

## Files

```text
proto_case.scad       parametric source for the prototype case
proto_base.stl        prototype body (print as-is, bottom on bed)
proto_lid.stl         prototype lid (engraved face up)
proto_preview.png / proto_layout.png   renders (layout = top view of bays)

concept_case.scad     parametric source (OpenSCAD, free: openscad.org)
concept_shell.stl     main body - print upside down (top face on bed), supports for lugs
concept_bottom.stl    skin-side sensor plate - snap-fit into shell
concept_preview.png / concept_bottom.png / concept_exploded.png   renders
```

## A. Prototype version — 105 x 75 x 24 mm

Single-level layout, every module in its own fenced bay, 20 mm internal height
for jumper-wire slack. Lid opens with 4x M2 self-tapping screws for debugging.

```text
Layout (top view):        rear
  [LTE reserve 26x28] [battery pocket] [BE-220 GPS, open sky above]
  [                 ] [              ] [TP4056 charge bay -> USB hole in +X wall]
  [ESP32, USB-C hole in -X wall] [MAX30102, floor window + shroud] [TMP117]
                            front (skin side)
```

- MAX30102: 9x7 through-window in the floor, external black light-shroud rim,
  centered where strap pressure is stable
- TMP117: 0.8 mm thermal membrane in the floor, placed diagonally opposite the
  LTE bay (max heat separation). Note: TP4056 bay is ~3.5 mm away - ignore
  temperature readings while charging
- LTE bay is oversized (26x28) because the module is TBD - M5Stamp CatM,
  Waveshare SIM7080G mini boards etc. all fit. When confirmed, edit
  lte_pos / lte_sz in proto_case.scad and re-export
- Lid: trigger button hole (GPIO4), reset access hole, 7 buzzer sound holes,
  engraved label. Strap: 26 mm velcro loops on both long sides
- Assembly shopping list: 4x M2x8 screws, 25 mm velcro, 6x6 tactile switch,
  TP4056 (USB-C version), hot glue / foam tape

## Key specs

```text
Body: 48 x 44 x 16 mm, rounded (r11), curved underside R45 to fit the wrist
Band: standard 24 mm watch band, integrated lugs, 1.3 mm spring bars
Internal stack (bottom to top):
  1. Skin plate: MAX30102 optical window 8x6 with black light-shroud ring groove,
     TMP117 thermal island (0.6 mm membrane), sensor pod raised 1.2 mm
     for stable skin pressure
  2. Integrated main PCB 38 x 33 (ESP32-C3 + LTE Cat-M + charge IC assumed on board)
  3. Battery 380 mAh (20x35x6) and BE-220 GPS (22x20x6) side by side on the PCB
     GPS sits under a 0.8 mm skylight in the top face (ceramic patch facing up,
     no metal above)
Controls: recessed top trigger button (accident-proof), side USB-C,
          buzzer micro-slits on the opposite side
```

## Design decisions carried over from our docs

- TMP117 thermally separated from LTE (LTE on PCB edge away from thermal island)
- MAX30102 flush to skin, light-blocked, centered pressure
- Battery never presses the optical sensor, no metal over the GPS patch
- SMS/debug requirements unchanged

## What is still parametric / open

- LTE module: assumed integrated on the main PCB. If we end up using a separate
  LTE breakout, this concept needs a thickness bump (16 -> ~18 mm) - one
  parameter change in the scad file.
- Charging: assumed on-board charge IC via the side USB-C.
- All dimensions are variables at the top of the .scad file - easy to tweak.

## Print notes

- Black PLA/PETG recommended (light blocking around the optical window matters)
- 0.2 mm layers, 3 walls; shell printed top-face-down with lug supports
