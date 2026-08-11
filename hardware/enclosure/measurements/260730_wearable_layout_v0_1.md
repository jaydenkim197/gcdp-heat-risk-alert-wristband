# 260730 Wearable Mechanical Layout v0.1

Date: 2026-07-30

Status: M4 working layout

Basis:

- Wrist circumference: 165 mm
- Wrist top straight width: 55 mm
- Main enclosure length target: 40 mm maximum
- Main enclosure width target: less than 50 mm
- Component envelopes from `260730_component_dimension_synthesis.md`

## 1. Architecture

Use three mechanically separated regions connected by flexible wiring:

```text
Outer wrist:
  Main electronics pod
  Separate BE-220 GPS pod
  LTE film antenna fixed flat along the outward-facing strap

Inner wrist:
  Combined MAX30102 + TMP117 sensor bar
```

Do not put every module in one rigid enclosure.

## 2. Main Electronics Pod

Contents:

- Stamp CatM S003
- ESP32-C3 SuperMini
- selected Li-Po battery
- 3 V active buzzer
- recessed trigger switch
- wiring junctions

Packing concept:

```text
Plan view:

  +--------------------+--------------------------+
  | Stamp CatM         | ESP32-C3 zone            |
  | 20.6 x 30.9        | battery stacked over ESP |
  |                    |                          |
  +--------------------+--------------------------+
```

The Stamp CatM is rotated so its 20.6 mm envelope is across the case and its
30.9 mm envelope runs along the case. The ESP32 and battery occupy the adjacent
region. The battery must not sit above the Stamp CatM.

First-pass outer target:

```text
With 150/220 mAh battery:
  approximately 49.5 x 33.5 x 14 to 15.5 mm

With 380 mAh battery:
  approximately 47 x 39 x 14.5 to 15.5 mm
```

Both versions remain within the current 50 x 40 mm plan limit. Final Z depends
on insulation, wire routing, and lid retention.

Required access:

- ESP32 USB-C side opening
- removable lid for SIM and wiring access
- recessed trigger on top
- buzzer openings on a side or lid
- IPEX coax exit toward the LTE antenna strap

## 3. GPS Pod

Contents:

- BE-220 only
- 4-wire power/UART cable

First-pass outer target:

```text
approximately 25 x 23 x 8 mm
plus a provisional 8 mm cable-exit zone
```

Placement:

- ceramic patch faces outward
- no battery, metal screw, or LTE antenna directly above it
- mount on the strap adjacent to, but mechanically separate from, the main pod
- verify the 4-pin connector allowance after delivery

## 4. Inner Sensor Bar

Contents:

- MAX30102
- TMP117 breakout
- shared 3V3, GND, SDA, and SCL harness

Packing:

```text
[ MAX30102 21.6 x 16.6 ] [ TMP117 26.0 x 18.4 ]
```

First-pass outer target:

```text
approximately 50 x 21 x 7.5 mm
```

Skin interface:

- MAX30102: centered 4 x 7 mm optical opening with a black light-blocking ring
- TMP117: centered approximately 4 x 4 mm thermal contact island
- board connectors and solder joints remain recessed inside the pod
- only the optical/thermal contact regions approach the skin plane
- use a soft gasket or TPU/silicone contact feature to stabilize pressure

The sensor bar connects to the main pod with one flexible 4-wire I2C harness.

## 5. Strap

Working width:

```text
22 mm
```

Reasons:

- accommodates the 20 mm-wide LTE film antenna
- carries the 4-wire sensor harness
- remains narrow relative to the 50 mm main body
- matches a common watch-strap class

LTE antenna placement:

- 50 x 20 x less than 1 mm film body
- adhere flat to the outward-facing strap
- keep away from the GPS patch
- route coax to the Stamp CatM without a sharp kink

## 6. Battery Decision Gate

Test order after delivery:

1. 220 mAh
2. 150 mAh
3. use the already proven 380 mAh if either smaller cell is unstable

Pass sequence:

```text
Battery-only power
-> Stamp CatM boot
-> network registration
-> BE-220 GPS operation
-> MAX30102 and TMP117 read
-> SMS transmission
-> no ESP reset or modem UART loss
```

## 7. Open Checks Before CAD Freeze

- Confirm actual 150/220 mAh pouch dimensions after delivery.
- Confirm BE-220 plug and cable-exit dimensions after delivery.
- Select a trigger switch within the reserved 8 x 8 x 6 mm envelope.
- Decide lid retention: two screws, four screws, or tape-assisted snap fit.
- Decide whether the inner sensor bar uses a printed TPU contact insert or a
  separate soft gasket.
