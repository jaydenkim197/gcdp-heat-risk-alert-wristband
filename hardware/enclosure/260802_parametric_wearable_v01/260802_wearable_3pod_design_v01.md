# 260802 Wearable 3-Pod Design v0.1

## 1. Fixed Inputs

- Wrist circumference: 165 mm
- Provisional wrist ellipse: 59 x 46 mm
- Calculated ellipse circumference: 165.57 mm
- Physical strap type: buckle-assisted hook-and-loop strap
- Strap width: 25 mm (2.5 cm)
- Provisional strap thickness: 2.0 mm
- Strap running clearance: 0.6 mm
- Normal start method: MAX30102 wear/contact detection; no external start button
- Enclosure wall, floor, lid, and structural features: 2.0 mm throughout

The wrist ellipse is only a packaging and curvature reference. Fit is adjusted by
the physical hook-and-loop strap, so the printed parts do not form a rigid cuff.

## 2. Architecture Decision

Final prototype architecture fixed on 2026-08-02: a narrow segmented strap,
not a watch body or a rigid cuff.

```text
Outer/top surface around wrist:
  [POWER link] -- flexible gap -- [GPS link] -- flexible gap -- [LTE link]

Skin surface:
  [MAX30102 contact tile] -- soft strap -- [TMP117 contact tile]

Outer return strap:
  [50 x 25 mm LTE film antenna sleeve]
```

Fixed placement rules:

- All rigid links follow the wrist circumference. Their long axis is tangent to
  the wrist, not parallel to the forearm.
- Maximum visible link width across the forearm is 25.2 mm. This matches the
  purchased 25 mm hook-and-loop strap and prevents a bold watch-like silhouette.
- The strap remains continuous and acts as the hinge. Printed links do not use
  mechanical hinge pins and do not form a rigid ring.
- Nominal flexible gap between adjacent upper links is 5 mm.
- GPS remains the dorsal center link with its ceramic patch facing outward.
- POWER sits on one dorsal flank and LTE on the opposite dorsal flank.
- The LTE antenna is not inside a pod. It sits in a thin non-metallic sleeve on
  the outward-facing return section of the strap.
- MAX30102 and TMP117 remain separate skin-contact tiles. Neither is stacked
  behind GPS, LTE, the battery, or the ESP32.

POWER link internal arrangement:

- 220 mAh battery lies flat at the bottom.
- ESP32-C3 keeps its long axis along the wrist circumference and is placed above
  the battery. The verified public slim case demonstrates an approximately
  20.8 mm external width in this orientation.
- The buzzer forms a tapered terminal region after the ESP32 instead of widening
  the link or becoming a third full vertical layer.
- USB-C faces a forearm-side service opening.
- Battery lead and external-charge connector remain accessible under the lid.

The enclosure language is a set of related rounded shingles: constant width,
large end radii, tapered lower edges, and no vertical box walls visible from the
normal viewing angle.

Wall policy:

- Use 2.0 mm uniformly for side walls, floor, lid, strap features, cable exits,
  and local structural regions.
- Do not introduce separate wall-thickness classes in the camp prototype.
- A 20.0 mm-wide module with 0.4 mm clearance per side and 2.0 mm walls produces
  a 24.8 mm outer width, remaining within the narrow 25 mm strap target.

## 3. Narrow-Link Target Envelopes

Axis convention: circumference length x forearm width x radial thickness.
These are v0.2 design targets, not final print-fit dimensions.

| Link | Target outer envelope (mm) | Notes |
| --- | ---: | --- |
| POWER: 220 mAh + ESP32 + buzzer | 35 x 22 x 15 max | ESP long axis follows wrist; narrow buzzer terminal after ESP |
| GPS: BE-220 only | 25 x 24 x 9.5 | Patch outward; no stacked electronics |
| LTE: Stamp CatM only | 34 x 24 x 9.5 | SIM and U.FL service access retained |
| MAX30102 contact tile | 24 x 19 x 4 target | Optical window and black light-blocking ring on skin side |
| TMP117 contact tile | 20 x 27.5 x 4 target | Board rotated across strap; connector removed for low profile |

Three upper links plus two 5 mm gaps occupy about 100 mm of wrist circumference.
The remaining circumference stays flexible for the sensor tiles, buckle, and fit
adjustment. The LTE film antenna uses the overlapping outer return strap and is
not counted as another rigid link.

Battery decision (fixed 2026-08-02):

- Use the TW-602025 220 mAh Li-Po for the wearable prototype.
- Physical battery measurement: 28 x 20 x 6 mm, 3.7 V.
- CAD axis convention: length = 28 mm, width = 20 mm, height = 6 mm.
- The 150 mAh battery provides too little packaging benefit to justify its lower
  capacity, so it is no longer the primary design candidate.
- The 380 mAh battery has already powered repeated bench tests without recharge,
  demonstrating that the prototype workload is modest, but its 35 mm length is
  rejected for the wearable enclosure because of size.
- The battery + ESP + buzzer pod must be revised from the current 150 mAh height
  assumption to the 220 mAh envelope before the fit print.

Buzzer allowance:

- Measured cylindrical body: 9.1 mm diameter x 5.9 mm high.
- CAD envelope: 9.7 mm diameter x 6.5 mm high, excluding full lead length.
- The battery + ESP pod has a separate 10.2 mm end zone for the buzzer, keeping
  the pod at the 40 mm wrist-length target without adding another vertical layer.

## 4. Lower Sensor Region

The previous rigid 51 mm sensor bar is rejected. Use two independent low-profile
contact tiles on the skin side of the continuous strap:

- MAX30102 optical tile near the lower center of the wrist.
- TMP117 tile offset from MAX30102 and away from POWER/LTE heat.

The next revision must add:

- MAX30102 optical opening and black light-blocking ring
- TMP117 thermal contact island
- soft gasket/TPU contact feature
- four-wire I2C cable exit and strain relief
- direct-soldered thin wires after removing protruding headers/connectors

## 5. Strap and Buckle Integration

- The existing buckle remains part of the purchased strap and is not printed.
- Each upper pod uses an open-ended, low-profile strap cradle.
- The free strap end passes through the existing buckle and folds back, matching
  the current tourniquet-style fastening method.
- The current 2.0 mm strap thickness is provisional. Measure the compressed and
  uncompressed thickness before the first fit print.

## 6. Exported CAD

Fusion-compatible STEP and print-oriented STL files are in `exports/`.

- Full 3-pod layout: `260802_layout_3pod_v01.step`
- Rejected 2-pod comparison: `260802_layout_2pod_comparison_v01.step`
- Wrist/strap reference: `260802_wrist_165mm_strap_25mm_reference.step`
- Individual base/lid files for battery + ESP, GPS, and Stamp CatM
- Lower sensor-bar envelope

The individual STL bodies were checked as watertight. The v0.1 lids are flat
fit-test covers without snap hooks; use tape-assisted retention for the first
physical layout test.

## 7. Milestone Status

| Milestone | Status | Result |
| --- | --- | --- |
| M1 Functional hardware proof | Complete | Sensors, GPS, Cat-M, SMS, battery operation previously demonstrated |
| M2 Mechanical dimensions | Mostly complete | 220 mAh battery fixed and measured; strap thickness and buckle dimensions remain |
| M3 Wearable architecture | Complete | Narrow segmented strap: 3 upper links + 2 skin tiles + return-strap antenna |
| M4 Parametric CAD blockout | Revision required | v0.1 proved volume; v0.2 must rotate links tangent to wrist and use 220 mAh dimensions |
| M5 Fit print and physical placement | Pending v0.2 | Print link bases, thread the 25 mm strap, place real modules |
| M6 Detail enclosure revision | Pending | Ports, wire exits, sensor openings, retention, and strain relief |

## 8. Immediate Next Work

1. Rebuild the Fusion v0.2 blockout with every upper link tangent to the wrist.
2. Revise POWER for the measured 28 x 20 x 6 mm battery, longitudinal ESP, and
   tapered buzzer terminal.
3. Add separate MAX30102 and TMP117 skin-contact tile envelopes.
4. Add the LTE antenna sleeve to the outward return strap.
5. Measure strap thickness and buckle outer dimensions.
6. Print only the link bases and sensor-tile carriers first.
7. Thread the actual strap, place real modules, and record cable and pressure issues.

## 9. Dual-Axis Hinge Reference Measurement (2026-08-03)

The selected reference STL uses a dual-axis hinge with an extra intermediate
piece. Each case-to-case connection therefore has two rotation axes rather than
one direct pivot axis.

Direct measurements from the reference STL, following the numbered hinge
segments 1 through 5 shown in the inspection image:

```text
Segment 1: 68.5
Segment 2: 97.25
Segment 3: 68.5
Segment 4: 97.25
Segment 5: 68.5
```

Observed pattern:

```text
68.5 / 97.25 / 68.5 / 97.25 / 68.5
```

The measured spacing between the corresponding reference objects is 3 mm.
These values are recorded as direct reference-STL observations. Final prototype
hinge dimensions and print clearances have not yet been frozen from these
measurements.
