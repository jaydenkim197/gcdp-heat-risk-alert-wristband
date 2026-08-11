# 260718 GPS / LTE Module Selection Update

Date: 2026-07-18  
Purpose: Record the updated module decision before continuing PCB and case design.

## 1. Decision Summary

The previous SIM7080G developer-board based approach is no longer the intended final prototype direction because the module/devkit size is too large for the wearable form factor.

Updated direction:

```text
GPS:
  Use Beitian BE-220 GPS module.

LTE:
  Use a smaller Cat-M LTE module instead of the large SIM7080G developer board.

Previous SIM7080G developer board:
  Do not use for the wearable prototype enclosure.
  Keep only as a proven test/reference platform for AT command, LTE, GNSS, and SMS behavior.
```

Note: the earlier SIM7080G tests are still valuable because they verified the alert pipeline:

```text
Sensor read -> location acquisition -> SMS alert transmission
```

However, the mechanical design target now changes from one large LTE/GNSS devkit to separated smaller GPS and LTE modules.

## 2. New GPS Module: Beitian BE-220

Reference material saved in:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\03_references\260718_BE-220
```

Reference files:

```text
260718_BE-220_datasheet.pdf
260718_BE-220_datasheet_cover.jpg
260718_BE-220_dimensions.jpg
260718_BE-220_overview.png
260718_BE-220_overview_product_parameter.jpg
260718_BE-220_pinout_led.jpg
260718_BE-220_specs.jpg
```

### BE-220 Main Specs

```text
Model: Beitian BE-220
Function: GPS module
Chip: M10050
Size: 22 mm x 20 mm x 6 mm
Weight: 5.3 g
Connector: 1.00 mm 4-pin connector
Supply voltage: DC 3.6 V to 5.5 V, typical 5.0 V
Current: about 15 mA at 5.0 V
Operating temperature: -40 C to +85 C
Storage temperature: -40 C to +105 C
```

### GNSS Support

```text
Default positioning system:
  GPS, BDS, GALILEO

Receiver type:
  GPS L1 C/A
  QZSS L1 C/A/S
  BDS B1I/B1C
  Galileo E1B/C
  SBAS L1 C/A: WAAS, EGNOS, MSAS, GAGAN

Augmentation:
  SBAS, QZSS
```

### Sensitivity / Accuracy

```text
Tracking and navigation: -166 dBm
Reacquisition: -160 dBm
Cold start: -148 dBm
Hot start: -160 dBm

Horizontal position accuracy: 2.0 m CEP
Velocity accuracy: 0.05 m/s
Dynamic heading: 0.3 deg
Time pulse: RMS 30 ns, 99% 60 ns
```

### Acquisition

```text
Cold start: 27 s
Hot start: 1 s
Aided start: 1 s
```

### Data Output

```text
Baud rate:
  4800 bps to 921600 bps
  Default 38400 bps

Electrical level:
  TTL level

Protocol:
  NMEA, UBX

NMEA messages:
  RMC, VTG, GGA, GSA, GSV, GLL

Update rate:
  0.25 Hz to 18 Hz
  Default 1 Hz

FLASH:
  Configuration can be changed and retained after power off.
```

### Pinout

```text
Pin 1: GND - Ground
Pin 2: TX  - Serial data output
Pin 3: RX  - Serial data input
Pin 4: VCC - DC 3.6 V to 5.5 V supply input, typical 5.0 V
```

### LEDs

```text
TX LED:
  Blue.
  Flashes when data output is present.

PPS LED:
  Red.
  Not bright when GPS is not fixed.
  Flashes after 3D positioning is achieved.
```

## 3. Impact on Architecture

Previous approach:

```text
SIM7080G developer board handled LTE + GNSS.
```

Updated approach:

```text
GPS location:
  BE-220 GPS module via UART.

LTE/SMS:
  Separate smaller Cat-M LTE module.

MCU:
  ESP32-C3 SuperMini remains the first-prototype controller unless changed later.
```

This means the ESP32-C3 now has to manage at least two serial devices:

```text
UART path 1:
  LTE Cat-M module for AT commands and SMS.

UART path 2:
  BE-220 GPS module for NMEA/UBX location data.
```

ESP32-C3 SuperMini pin pressure increases because it also needs:

```text
I2C SDA/SCL for TMP117 and MAX30102
Buzzer pin
LTE PWRK/control pin
Standalone trigger pin
LTE UART RX/TX
GPS UART RX/TX
```

Design implication:

- The next schematic must reserve a second UART path for GPS.
- If hardware UART availability or pin count becomes tight, GPS can be read through a software serial style approach only if reliable at the selected baud rate.
- Since BE-220 default is 38400 bps, it is more forgiving than very high-speed serial, but hardware UART is still preferable.

## 4. Firmware Impact

Existing firmware assumptions:

```text
GNSS location came from SIM7080G AT commands.
Commands used SIM7080G GNSS functions.
```

Required firmware change:

```text
Remove dependency on SIM7080G GNSS commands for final prototype.
Add BE-220 GPS serial parser.
Parse NMEA sentences, probably GGA/RMC first.
Keep LTE module focused on network registration and SMS.
```

Recommended first GPS firmware target:

```text
Read serial at BE-220 default 38400 bps.
Detect NMEA lines.
Parse GGA or RMC.
Extract:
  latitude
  longitude
  fix validity
  satellite/fix quality if available
Use last valid fix in alert SMS.
```

Alert SMS target format remains:

```text
HR: <bpm>, TEMP: <temperature>, LOC: <latitude>, <longitude>
```

## 5. PCB Design Impact

The first PCB should no longer be centered around the large SIM7080G developer board.

New PCB concerns:

```text
Add BE-220 4-pin connector footprint:
  GND
  TX
  RX
  VCC

Provide GPS power:
  3.6 V to 5.5 V allowed
  Typical 5 V
  About 15 mA

Route GPS UART to ESP32-C3.

Reserve separate LTE UART.

Keep GPS antenna ceramic patch clear of copper, battery, and metal nearby where possible.
```

Mechanical footprint:

```text
BE-220 module body: 22 mm x 20 mm x 6 mm
```

The GPS module is smaller than the earlier SIM7080G devkit but still has a ceramic patch antenna. Its top face orientation and keepout matter.

## 6. Case Design Impact

The case design should now treat GPS and LTE as separate RF blocks.

### GPS Placement

Recommended:

```text
Place BE-220 near the outer/top surface of the case.
Keep ceramic patch facing outward/upward.
Avoid placing battery directly over the patch antenna.
Avoid metal screws or dense copper immediately above/around the patch.
Do not bury it under thick or conductive material.
```

Reason:

- GPS reception is sensitive to antenna orientation and obstruction.
- The BE-220 includes a patch-style antenna area, so its physical orientation is part of the electrical design.

### LTE Placement

Recommended:

```text
Place LTE Cat-M module away from TMP117.
Keep LTE antenna path away from the body side where possible.
Provide RF cable or antenna clearance depending on selected LTE module.
```

Reason:

- LTE transmission can create heat and current spikes.
- LTE antenna performance can degrade if blocked by battery, skin, or metal.

### Sensor Placement Still Has Priority

The mechanical priorities remain:

```text
MAX30102:
  Must be flush to skin with light blocking.

TMP117:
  Must be near skin but thermally separated from LTE heat.

Battery:
  Must be mechanically protected.

GPS:
  Must face outward and be kept clear.

LTE:
  Must have antenna clearance and heat separation.
```

## 7. What Changes From the 260713 Team Document

The 260713 document described a working prototype using a SIM7080G LTE/GNSS path.

Updated interpretation:

```text
Keep:
  Sensor validation results.
  Battery test results.
  SMS alert workflow.
  ESP32-C3 SuperMini first-prototype controller assumption.
  Case requirements for MAX30102, TMP117, battery, buzzer, trigger, and debug access.

Change:
  Do not use the large SIM7080G devkit as the wearable communication/GNSS module.
  Split GPS and LTE into separate smaller modules.
  Add BE-220 GPS as the selected GPS module.
  Select/use a smaller Cat-M LTE module for SMS.
```

## 8. Immediate Next Steps

Hardware:

- Confirm the exact Cat-M LTE module model and pinout.
- Confirm BE-220 connector orientation and cable availability.
- Decide GPS UART pins on ESP32-C3 SuperMini.
- Decide LTE UART pins on ESP32-C3 SuperMini.

Firmware:

- Add BE-220 NMEA serial read test.
- Verify GPS fix and NMEA parse.
- Modify alert SMS to use BE-220 location.
- Keep LTE SMS sending independent from GPS parsing.

Case:

- Update block layout with BE-220 22 mm x 20 mm x 6 mm module.
- Put GPS on outer/top side.
- Keep MAX30102 and TMP117 on skin-facing side.
- Avoid placing BE-220 under battery or LTE module.

