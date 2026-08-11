# GCDP Wristband Case Modeling

Updated: 2026-07-25

Purpose: Fusion workspace for the wearable prototype case/blockout.

## Current Modeling Goal

Create a practical blockout first, not a polished final enclosure.

The prototype will likely be:

- wired modules
- fixed inside a TPU/printed holder
- covered or enclosed enough for wearable presentation
- supported by a presentation-only PCB design image later

## Folder Map

- `fusion`: Fusion source/exported archive files.
- `exports`: STL/STEP/3MF outputs.
- `references`: reference images imported into Fusion or used for modeling.
- `measurements`: measurement tables and photos.
- `renders`: screenshots/renders for reports and presentation.

## Fixed Modules

- ESP32-C3 SuperMini
- M5Stack Stamp CatM S003
- Beitian BE-220 GPS/GNSS
- TMP117 breakout
- MAX30102 breakout
- 3 V active buzzer
- TW-602035 3.7 V 380 mAh Li-Po battery

## Blockout Priority

1. Skin-facing sensor region: MAX30102 optical window and TMP117 contact zone.
2. Top/outward GPS region: BE-220 patch antenna faces outward/upward.
3. LTE region: Stamp CatM and LTE antenna kept away from GPS patch and skin side where possible.
4. Battery protection: avoid sharp compression and provide retention.
5. Debug access: ESP32-C3 USB-C remains reachable.
6. Wire strain relief: every module wire exit needs room.

## First Fusion Components

Create one component per object:

- `WRIST_REFERENCE`
- `CASE_BASE`
- `CASE_LID`
- `ESP32_C3_SUPERMINI_BLOCK`
- `STAMP_CATM_BLOCK`
- `BE220_GPS_BLOCK`
- `BATTERY_BLOCK`
- `MAX30102_BLOCK`
- `TMP117_BLOCK`
- `BUZZER_BLOCK`
- `BUTTON_BLOCK`
- `WIRE_CHANNELS`

