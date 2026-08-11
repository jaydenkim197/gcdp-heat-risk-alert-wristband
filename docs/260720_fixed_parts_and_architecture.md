# 260720 Fixed Parts and Prototype Architecture

Date: 2026-07-20

Purpose: 현재 확정된 사용 부품과 프로토타입 구조를 기록한다. 팀원 노티용 문서는 제거하고, 기록용 로그는 보존한다.

## 1. Current Direction

현재 방향은 PCB-first가 아니라 배선형 wearable prototype이다.

```text
ESP32-C3 SuperMini
  + M5Stack Stamp CatM S003
  + Beitian BE-220 GPS module
  + TMP117 module
  + MAX30102 module
  + active buzzer
  + TW-602035 Li-Po battery
  + TPU/printed wristband enclosure
```

이전 SIM7080G large developer board는 최종 착용형 구조에 쓰지 않는다. 다만 LTE/SMS/GNSS/AT command 검증 기록으로는 보존한다.

## 2. Fixed Main Components

| Block | Fixed Part | Status | Notes |
| --- | --- | --- | --- |
| MCU | ESP32-C3 SuperMini | Fixed | Final prototype controller. USB-C remains accessible for upload/debug. |
| LTE/SMS | M5Stack Stamp CatM S003 | Fixed | Communication-only Cat-M. No GNSS by itself. UART 115200. MicroSIM/on-module SIM path. |
| GPS/GNSS | Beitian BE-220 GPS/GNSS module | Fixed | Separate GPS module via UART/NMEA. |
| Temperature | TMP117 breakout/module | Fixed | I2C address 0x48. Existing module retained. |
| Optical / wear | MAX30102 breakout/module | Fixed | I2C address 0x57. Existing module retained. |
| Alert output | 3 V active buzzer | Fixed | GPIO controlled. Existing buzzer class retained. |
| Battery | TW-602035 Li-Po 3.7 V 380 mAh | Fixed | 20 x 35 x 6 mm, 1.25 mm 2-pin connector. External charging only. |
| Trigger | Tactile switch or exposed pads | Fixed | Active-low standalone trigger to ESP32 GPIO4. |
| Enclosure | TPU printed band/holder first | Fixed direction | Silicone casting deferred unless needed for sensor contact pad. |

## 3. BE-220 GPS Module

Source folder:

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

Confirmed specs:

```text
Model: Beitian BE-220
Chip: M10050
Size: 22 x 20 x 6 mm
Weight: 5.3 g
Supply: DC 3.6 V to 5.5 V, typical 5.0 V
Current: about 15 mA at 5.0 V
Connector: 1.00 mm 4-pin
Default baud: 38400 bps
Protocol: NMEA, UBX
NMEA: RMC, VTG, GGA, GSA, GSV, GLL
Default update: 1 Hz
```

Pinout:

```text
Pin 1: GND
Pin 2: TX  serial data output
Pin 3: RX  serial data input
Pin 4: VCC 3.6 V to 5.5 V
```

Initial wiring:

```text
BE-220 GND -> ESP32 GND
BE-220 TX  -> ESP32 GPS_RX
BE-220 RX  -> ESP32 GPS_TX or NC for first read-only test
BE-220 VCC -> 5 V preferred for first test
Baud        38400
```

Case placement:

```text
Patch antenna face outward/upward.
Do not place battery over the patch antenna.
Keep away from LTE antenna/module as much as possible.
Avoid metal screws or dense copper above/around GPS.
```

## 4. LTE / Cat-M Module

Fixed direction:

```text
Use the small Cat-M module, not the large SIM7080G developer board.
Current selected module: M5Stack Stamp CatM S003.
```

Known module facts from saved notes:

```text
Based on SIM7080G.
Cat-M/NB-IoT communication.
No GNSS by itself.
UART: 115200 8N1.
MicroSIM on module.
LTE antenna via IPEX/SMA cable path.
Product size about 30.1 x 20.1 x 5.5 mm.
External connector includes TX, RX, GND, 3V3, 5V, ANT on reference drawing.
```

Design meaning:

```text
LTE handles network registration and SMS only.
GPS location comes from BE-220.
ESP32 must manage separate LTE UART and GPS UART.
```

## 5. ESP32-C3 SuperMini Pin Allocation

Final SuperMini target:

```text
GPIO8  -> I2C SDA for TMP117/MAX30102
GPIO9  -> I2C SCL for TMP117/MAX30102
GPIO20 -> LTE UART RX, ESP receives from Cat-M TX
GPIO21 -> LTE UART TX, ESP sends to Cat-M RX
GPIO10 -> Buzzer
GPIO5  -> LTE PWRK / module wake control if needed
GPIO4  -> Standalone trigger, active-low to GND
GPIO6/GPIO7 or GPIO3/GPIO2 -> GPS UART candidate, pending firmware test
```

Current firmware has both XIAO and SuperMini environments. PCB/case documentation should use SuperMini final pinout, while current bench test history may still mention XIAO pins.

## 6. Sensor Modules

TMP117:

```text
Use module/breakout, not bare IC.
VIN -> 3V3
GND -> GND
SDA/SCL -> shared I2C
INT/ADDR unused.
Address: 0x48
```

MAX30102:

```text
Use module/breakout, not bare IC.
VIN -> 3V3
GND -> GND
SDA/SCL -> shared I2C
INT/IRD/RD unused.
Address: 0x57
```

Mechanical note:

```text
MAX30102 must be flush to skin and light-blocked.
TMP117 must be near skin but separated from LTE heat.
Use strain relief on all soldered sensor wires.
```

## 7. Power

Battery:

```text
TW-602035 Li-Po
Nominal: 3.7 V
Capacity: 380 mAh
Energy: 1.406 Wh
Size: about 20 x 35 x 6 mm
Connector: 1.25 mm 2-pin
```

Confirmed from logs:

```text
Battery-powered prototype alert path: PASS.
ESP32-C3 external power with USB debug: PASS.
SIM7080G developer board battery-side power: PASS.
```

Current prototype policy:

```text
No onboard charger.
Charge battery externally.
Do not connect Li-Po directly to ESP32 3V3.
For the wired prototype, use the tested power path and keep grounds common.
```

If the selected Cat-M module requires 5 V, provide a suitable 5 V supply/boost path for that module. Do not assume every Cat-M board accepts raw Li-Po unless its exact input pin spec confirms it.

## 8. Buzzer and Trigger

Buzzer:

```text
3 V active DC buzzer
Operating range: about 2 V to 5 V
Rated current max: about 30 mA
ESP32 GPIO10 control
Use transistor/MOSFET driver if packaging allows; direct GPIO only for temporary bench tests.
```

Trigger:

```text
GPIO4 active-low to GND.
Hold low for about 1.5 s to start standalone alert flow.
Use tactile switch or exposed pads in the case.
```

## 9. Enclosure / Case

Fixed direction:

```text
TPU printed holder/band first.
Silicone casting is optional and deferred.
```

Recommended layout:

```text
Skin side:
  MAX30102 optical window
  TMP117 thermal contact area

Outer/top side:
  BE-220 GPS patch antenna facing outward
  LTE antenna with clearance

Main body:
  ESP32-C3 SuperMini
  Cat-M module
  battery
  buzzer
  trigger button/pads
  wire strain relief
```

## 10. Consumables / Tools

Recommended:

```text
A-888 leaded solder paste or normal solder wire + flux
RMA-223 flux
Heat shrink tube
Thin flexible wires
Kapton/electrical tape
Hot glue / UV resin / epoxy for strain relief
TPU filament
Optional black silicone or foam ring for MAX30102 light blocking
```

Not required:

```text
Buying every solder paste type.
Silicone full-body casting for the first wearable prototype.
PCB antenna.
Bare ESP32-C3 / bare SIM7080G module integration.
```

## 11. Preserved / Removed Documents

Preserve as logs:

```text
01_docs/260708_hardware_bringup_log.md
01_docs/260709_lte_sms_integrated_firmware_log.md
01_docs/260713_battery_standalone_alert_test_log.md
```

Removed as team-notice style document:

```text
removed: team notification draft, not a record log
```

Current authoritative design documents:

```text
01_docs/260718_gps_lte_module_selection_update.md
01_docs/260720_fixed_parts_and_architecture.md
```

