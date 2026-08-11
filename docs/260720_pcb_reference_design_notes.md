# Current Design Notes - Wired Wearable Prototype

Date: 2026-07-20

## Architecture

```text
Sensors:
  TMP117 + MAX30102 on shared I2C

MCU:
  ESP32-C3 SuperMini

LTE/SMS:
  M5Stack Stamp CatM S003, SIM7080G-based, communication only

GPS:
  Beitian BE-220 GPS/GNSS module, separate UART

Power:
  TW-602035 Li-Po battery, external charging

Case:
  TPU printed wearable holder/band
```

## Fixed Module Data

### ESP32-C3 SuperMini

```text
Use as development-board/module controller.
USB-C must remain accessible.
Do not move to bare ESP32-C3 chip for this prototype.
```

Target pin plan:

```text
GPIO8  -> I2C SDA
GPIO9  -> I2C SCL
GPIO20 -> LTE UART RX
GPIO21 -> LTE UART TX
GPIO10 -> Buzzer
GPIO5  -> LTE PWRK/wake if needed
GPIO4  -> Standalone trigger, active-low to GND
GPIO6/GPIO7 or GPIO3/GPIO2 -> GPS UART candidate
```

### Cat-M LTE/SMS Module

```text
Use M5Stack Stamp CatM S003 rather than the large SIM7080G developer board.
Expected role: LTE registration and SMS.
No GNSS dependency.
UART: 115200 8N1.
Keep LTE antenna clear of battery/body/metal as much as possible.
```

### BE-220 GPS

```text
Model: Beitian BE-220
Size: 22 x 20 x 6 mm
VCC: 3.6 V to 5.5 V, typical 5.0 V
Current: about 15 mA at 5 V
Default baud: 38400 bps
Protocol: NMEA/UBX
Pinout: GND, TX, RX, VCC
```

Placement:

```text
Patch antenna faces outward/upward.
Do not place under battery.
Avoid metal above/around GPS patch.
Keep separate from LTE antenna where possible.
```

### TMP117

```text
Use existing breakout/module.
I2C address: 0x48.
Use VIN/GND/SDA/SCL only.
Place near skin contact, away from LTE heat.
```

### MAX30102

```text
Use existing breakout/module.
I2C address: 0x57.
Use VIN/GND/SDA/SCL only.
Must be flush to skin and light-blocked.
```

### Battery

```text
TW-602035 Li-Po
3.7 V nominal
380 mAh
20 x 35 x 6 mm
1.25 mm 2-pin connector
External charging only
```

### Buzzer / Trigger

```text
Buzzer: 3 V active buzzer on GPIO10.
Trigger: GPIO4 active-low to GND, hold about 1.5 s for standalone alert.
```

## Preserved Validation Results

The record logs under the N: workspace remain authoritative for validation history:

```text
260708_hardware_bringup_log.md
260709_lte_sms_integrated_firmware_log.md
260713_battery_standalone_alert_test_log.md
```

Current verified functions:

```text
TMP117 temperature read
MAX30102 RED/IR and wear detection
Buzzer feedback
LTE registration
SMS transmission
GNSS/location acquisition on previous SIM7080G test path
Battery-powered standalone alert flow
```

Next firmware change:

```text
Use BE-220 NMEA GPS data instead of SIM7080G GNSS commands for the final wired wearable.
```
