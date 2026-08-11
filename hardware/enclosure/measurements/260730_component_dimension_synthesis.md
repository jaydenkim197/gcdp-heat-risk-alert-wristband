# 260730 Component Dimension Synthesis

Date: 2026-07-30

Purpose: Establish the mechanical input dimensions for the camp wearable
prototype from saved datasheets, vendor drawings, and exact purchased-part
references.

The previous `proto_case` geometry is not a design input for this document.

## 0. Primary Local Sources

- `06_3d_case/references/datasheets/esp32_c3_supermini_board_datasheet.pdf`
- `05_pcb_altium/datasheets/m5stack_stamp_catm_s003.pdf`
- `03_references/260718_BE-220/260718_BE-220_dimensions.jpg`
- `03_references/260718_BE-220/260718_BE-220_datasheet.pdf`
- `03_references/260712_sensor_refs/260712_adafruit_sensor_ref_01.png`
- `04_media/260713_case_refs/260713_case_ref_03.png`
- `04_media/260713_case_refs/260713_case_ref_04.png`
- `04_media/260713_case_refs/260713_case_ref_05.png`
- `03_references/260730_batteries/260730_TW-402025_150mAh_specs.png`
- `03_references/260730_batteries/260730_TW-602025_220mAh_specs.jpg`

## 1. Dimension Status

| Part | Documented body size (mm) | Status | Mechanical notes |
| --- | ---: | --- | --- |
| ESP32-C3 SuperMini | 22.52 x 18.00 x 4.0 measured | Physical measurement confirmed | The bare PCB is less than 1 mm thick. The USB-C connector is the highest feature and extends less than 1 mm beyond the board edge. Use a fresh board without pin headers and solder thin flexible wires directly to the required pads. |
| M5Stack Stamp CatM S003 | 30.3 x 20.0 x 6.0 measured | Physical measurement confirmed | The SIM-holder region is the tallest point. Most of the remaining body is about 4 mm high. A thin removable plastic armor is installed. Keep the armor for the first camp prototype unless enclosure thickness becomes critical. |
| Beitian BE-220 GPS | 22.0 x 20.0 x 6.25 maximum measured | Physical measurement confirmed after delivery | The module body is effectively occupied by the receiver and ceramic antenna. Treat 6.0 mm as the nominal body height and 6.25 mm as the measured worst-case fit height. Ceramic patch faces outward. |
| Adafruit TMP117 breakout | 25.40 x 17.78 x 5.0 measured | Physical measurement confirmed | Maximum height includes the installed 4-pin connector. Without the connector, the tallest board feature is about 2.6 mm. The approximately 2 x 2 mm TMP117 chip is centered on the PCB. Keep the connector for the camp prototype because the 5 mm total height remains acceptable and serviceability is useful. |
| MAX30102 breakout currently used | 21 x 16 x 3.0 overall measured | Physical measurement confirmed | PCB thickness is about 1.2 mm. The optical package is about 6 x 3 mm and centered on the PCB. With PCB axes X=21 and Y=16, the optical package is X=3 and Y=6. Existing pin-header protrusion is not included in this body size. |
| TW-602035 380 mAh Li-Po | 35 x 20 x 6 measured | Tested, not selected | This battery powered repeated bench tests without recharge, but its 35 mm length is rejected for the wearable prototype. Retain as a bench-test battery. |
| TW-602025 220 mAh Li-Po | 28 x 20 x 6 measured | Selected and physically confirmed | Fixed wearable battery as of 2026-08-02. Physical measurement overrides the nominal 20 x 25 x 6 mm product listing. CAD convention is length 28, width 20, height 6 mm. Wire length is adjustable. |
| TW-402025 150 mAh Li-Po | 25 x 20 x 4 nominal | Alternate only | Its plan dimensions are effectively the same as the selected 220 mAh cell, while the capacity is lower. It is no longer the primary enclosure target. |
| 3 V active buzzer FQ-013 / TMB09A03 | 9.1 diameter x 5.9 body height measured | Physical measurement confirmed | Cylindrical body. Vendor drawing gives 5.6 mm lead spacing and about 6.0 +/- 0.5 mm lead length. Leads may be trimmed after soldering. |
| Start/debug trigger | No external button required | Architecture decision | Normal operation starts from stable MAX30102 wear/contact detection. Retain only an optional internal test pad or firmware debug trigger for forced execution during development. |
| Quectel YF0006AA LTE antenna | 50 x 25 x 0.13 antenna body; 30 mm cable | Official datasheet confirmed | The distributor screenshot says 50 x 20 mm, but Quectel V2.3 specifies 50 x 25 mm. Use the official larger width until the actual part is measured. U.FL termination; place on an outward-facing non-metallic surface, not beneath the battery or against skin. |

## 2. CAD Block Dimensions

These are the recommended first-pass CAD envelope values. They include small
body tolerance only, not cable bend space or general assembly clearance.

| Part | CAD body envelope (mm) |
| --- | ---: |
| ESP32-C3 SuperMini | 23.0 x 18.5 x 4.6 |
| Stamp CatM S003 | 30.9 x 20.6 x 6.6 |
| BE-220 GPS | 22.6 x 20.6 x 6.9 |
| TMP117 breakout | 26.0 x 18.4 x 5.6 |
| MAX30102 breakout | 21.6 x 16.6 x 3.5, excluding full pin-header protrusion |
| 380 mAh battery pocket (bench reference only) | 36 x 21 x 6.8 |
| 220 mAh battery pocket (selected, first pass) | 28.8 x 20.8 x 6.8 |
| 150 mAh battery pocket (alternate only) | 26 x 21 x 4.8 |
| Active buzzer | 9.7 diameter x 6.5, excluding full lead length |
| Optional internal test pad | PCB pad only; no enclosure press feature |
| LTE film antenna | 50 x 25 x 0.13, plus 30 mm coax routing envelope |

Do not add final print-fit clearance twice. The values above are component
envelopes; enclosure pocket clearance is added later according to the printer,
material, and retention method.

## 3. Required Physical Measurements

Measure in this order because each item can change the enclosure architecture.

1. Selected 220 mAh battery:
   confirm the first printed pocket fit and inspect protection-board bulge and
   wire-exit compression before the final retention feature is added.
2. LTE antenna:
   measure the actual film width to resolve the 20 mm distributor listing versus
   the 25 mm official Quectel datasheet value.

## 4. Mechanical Constraints Already Fixed

- MAX30102 optical face must contact skin and be surrounded by a light-blocking ring.
- TMP117 must be skin-adjacent and thermally separated from Stamp CatM and power heat.
- BE-220 ceramic patch must face outward with no battery or metal directly above it.
- The Li-Po pouch must not be compressed by the lid, screws, or wrist curvature.
- ESP32-C3 USB-C must remain accessible until firmware is stable.
- Every soldered wire exit needs strain relief.
- SIM access and the LTE antenna connection must remain serviceable for the camp prototype.
- Use the 220 mAh cell as the prototype packaging baseline. Complete a final
  battery-only end-to-end demonstration after the mechanical assembly is wired.

## 5. Wrist and Wearable Limits

Physical wearer measurements:

```text
Wrist circumference: 165 mm
Top straight width: 55 mm
Comfortable enclosure length along the wrist: 40 mm
Maximum enclosure width across the wrist: less than 50 mm
```

Working mechanical targets:

```text
Main electronics pod: no larger than 50 x 40 mm in plan
Strap width: 22 mm provisional
Reason for 22 mm: suitable for the compact sensor harness. The official 25 mm-wide
LTE film antenna may need to bridge the strap edges or occupy an enclosure surface.
```

## 6. Current Gate

Milestone M2 is complete only when every `physical measurement required` item
has a measured value or an explicit prototype allowance.
