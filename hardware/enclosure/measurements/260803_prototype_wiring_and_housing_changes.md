# 260803 Prototype Wiring And Housing Changes

Date: 2026-08-03

Status: wiring-layout freeze before final enclosure printing

## 1. Recommended Physical Order

Arrange the rigid housings in this order around the wrist:

```text
Stamp CatM -- ESP32-C3 + 220 mAh battery -- BE-220 GPS -- MAX30102 -- TMP117
```

Reasons:

- The Stamp CatM high-current power path remains short.
- The sensor bus crosses only the GPS housing before reaching MAX30102, while
  TMP117 remains farthest from LTE and main-electronics heat.
- The GPS housing can carry the sensor bus through without placing the TMP117
  next to the modem.
- The buzzer remains inside the ESP32-C3/battery housing.

## 2. Fixed ESP32-C3 SuperMini Pin Map

Use the tested `esp32c3_supermini` firmware environment.

| Function | ESP32-C3 pin | Remote connection |
|---|---:|---|
| I2C SDA | GPIO8 | MAX30102 SDA and TMP117 SDA |
| I2C SCL | GPIO9 | MAX30102 SCL and TMP117 SCL |
| LTE RX | GPIO20 | Stamp CatM TX |
| LTE TX | GPIO21 | Stamp CatM RX |
| GPS RX | GPIO6 | BE-220 TX |
| GPS TX | GPIO7 | BE-220 RX |
| Buzzer | GPIO10 | Active buzzer positive/signal |
| Sensor supply | 3V3 | MAX30102 VIN and TMP117 VIN |
| Common return | GND | All modules |

The delivered BE-220 was experimentally identified as `115200 baud`, even
though the vendor listing stated `38400 baud`.

## 3. Module Wiring

### Stamp CatM

```text
Stamp 5V  -> dedicated modem power pair from the main power junction
Stamp GND -> common GND
Stamp TX  -> ESP32-C3 GPIO20
Stamp RX  -> ESP32-C3 GPIO21
Stamp 3V3 -> no external connection
Stamp RF  -> LTE antenna coax
```

Do not externally connect both the Stamp `5V` and `3V3` pins. The board's
onboard 3.3 V rail was previously measured while the board was supplied from
5 V.

The Stamp-based battery-only full-system test is still pending in the existing
test log. Keep this power pair detachable until the final supply method passes
boot, network registration, and SMS transmission from the 220 mAh battery.

### BE-220 GPS

```text
BE-220 VCC -> main raw power rail
BE-220 GND -> common GND
BE-220 TX  -> ESP32-C3 GPIO6
BE-220 RX  -> ESP32-C3 GPIO7
```

The module input specification is 3.6-5.5 V. The current physical unit passed
NMEA and outdoor-fix testing when powered from the ESP USB 5 V rail. Battery
operation must be repeated after the final harness is assembled.

### MAX30102 And TMP117

Both sensors share one short four-wire I2C trunk:

```text
ESP 3V3   -> MAX30102 VIN -> TMP117 VIN
ESP GND   -> MAX30102 GND -> TMP117 GND
ESP GPIO8 -> MAX30102 SDA -> TMP117 SDA
ESP GPIO9 -> MAX30102 SCL -> TMP117 SCL
```

Unused pins:

```text
MAX30102 INT, RD, IRD -> not connected
TMP117 INT            -> not connected
TMP117 ADDR           -> retain breakout default
```

The physical bus order is ESP -> MAX30102 -> TMP117. Make the branch inside the
MAX30102 housing using insulated solder joints or a compact four-line splice.

### Active Buzzer

Keep the buzzer local to the ESP/battery housing:

```text
Buzzer + -> GPIO10
Buzzer - -> GND
```

## 4. Power Junction

Use a parallel/star junction inside the ESP/battery housing. Do not daisy-chain
the supply through module pads.

```text
220 mAh Li-Po connector
        |
        +-- ESP32-C3 5V/GND
        +-- Stamp CatM power pair
        +-- BE-220 VCC/GND

ESP32-C3 regulated 3V3
        +-- MAX30102 VIN
        +-- TMP117 VIN
```

The same two-pin battery connector is the master disconnect and external-charge
interface. Charging requires the battery to be physically disconnected from the
wearable and connected to the separate charger. Do not expose bare charging
contacts.

Use the purchased two-pin harness for battery and modem power. Use the purchased
SH1.0 four-wire harness for I2C and UART signals. Keep the modem power pair as
short as practical.

## 5. Housing Changes Before Final Print

### ESP32-C3 And Battery Housing

- Add a recessed opening for the two-pin battery disconnect connector.
- Add an internal strain-relief channel immediately after the battery splice.
- Add one four-wire exit toward the Stamp housing.
- Add two cable lanes toward the GPS/sensor side: GPS UART/raw power and the
  sensor 3V3/I2C trunk.
- Keep all splices above an insulated floor and away from the Li-Po pouch.
- Retain USB-C access for firmware upload and debugging.

### Stamp CatM Housing

- Add one four-wire entry for power, GND, TX, and RX.
- Add a rounded coax exit independent of the electrical-wire opening.
- Provide at least a 10 mm coax bend radius and no hinge pinch point.
- Keep the SIM and antenna connector serviceable.

### GPS Housing

- Add one entry facing the ESP housing for VCC, GND, TX, RX, and the sensor bus.
- Add one four-wire exit facing the MAX30102 housing for 3V3, GND, SDA, and SCL.
- Use separate shallow channels for GPS wiring and the passing sensor trunk.
- Keep every wire below the ceramic patch perimeter and do not cross its top.

### MAX30102 Housing

- Add a four-wire input from the GPS/ESP direction.
- Add a four-wire output toward TMP117.
- Add a protected internal splice pocket away from the optical package.
- Do not route wiring across the optical opening or skin-contact frame.

### TMP117 Housing

- Add one four-wire input only; this is the end of the I2C trunk.
- Place the wire strain relief away from the central thermal-contact island.
- Avoid a thick wire bundle directly behind the temperature sensor.

## 6. Hinge-Crossing Rules

- Do not route wires through a hinge pin or between knuckle end faces.
- Place each cable exit beside the pivot axis, toward the outward-facing surface.
- Provide a free loop of 5-8 mm between adjacent housings.
- Use rounded cable-slot edges with at least `R1.0 mm`.
- Recommended open cable notch for one four-wire harness: approximately
  `5.0 x 2.5 mm`; verify against the actual harness before printing.
- Where two harnesses pass the same joint, use two separate notches instead of
  compressing eight wires into one slot.
- Exercise the full hinge angle before fixing the wires with Kapton tape.
- Add strain relief at least 5 mm inside each housing so bending occurs in the
  free loop, not at a solder joint.

## 7. Assembly And Test Order

1. Print a cable-notch and hinge coupon before the complete housing set.
2. Route unpowered harnesses and check the complete hinge range.
3. Verify no pinching, insulation damage, or sensor-face obstruction.
4. Connect battery power with every module unplugged.
5. Add ESP32-C3 and verify regulated 3.3 V.
6. Add MAX30102 and TMP117 and run `I2CSCAN` and `MEASURE`.
7. Add BE-220 and run `GPSRAW`, `GPSPROBE`, and outdoor `GPS` fix.
8. Add Stamp CatM and verify `AT`, network registration, and one controlled SMS.
9. Disconnect USB and repeat the complete sequence from the 220 mAh battery.

Do not permanently close the housings until step 9 passes.
