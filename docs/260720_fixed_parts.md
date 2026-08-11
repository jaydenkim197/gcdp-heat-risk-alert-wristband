# GCDP Fixed Parts - 2026-07-20

This is the local temp copy of the current fixed component list. The main architecture note is:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\01_docs\260720_fixed_parts_and_architecture.md
```

## Fixed Prototype Components

```text
MCU:
  ESP32-C3 SuperMini

LTE/SMS:
  M5Stack Stamp CatM S003
  Used for LTE registration and SMS only

GPS:
  Beitian BE-220 GPS/GNSS module
  VCC 3.6~5.5 V, typical 5 V
  UART default 38400 bps
  Pinout: GND / TX / RX / VCC

Sensors:
  TMP117 breakout/module, I2C 0x48
  MAX30102 breakout/module, I2C 0x57

Alert output:
  3 V active buzzer

Battery:
  TW-602035 Li-Po, 3.7 V, 380 mAh, 20 x 35 x 6 mm, 1.25 mm 2-pin

Trigger:
  GPIO4 active-low tactile switch or exposed pads

Case:
  TPU printed band/holder first
  Optional black silicone/foam only around MAX30102 light-blocking area
```

## SuperMini Pin Plan

```text
GPIO8  -> I2C SDA
GPIO9  -> I2C SCL
GPIO20 -> LTE UART RX
GPIO21 -> LTE UART TX
GPIO10 -> Buzzer
GPIO5  -> LTE PWRK/wake if needed
GPIO4  -> Standalone trigger to GND
GPIO6/GPIO7 or GPIO3/GPIO2 -> BE-220 GPS UART candidate
```

## BE-220 Quick Wiring

```text
Pin 1 GND -> ESP32 GND
Pin 2 TX  -> ESP32 GPS_RX
Pin 3 RX  -> ESP32 GPS_TX or NC for first read-only test
Pin 4 VCC -> 5 V preferred for first test
Baud      -> 38400
```

## Current Architecture

```text
TMP117/MAX30102 -> ESP32-C3 SuperMini via I2C
BE-220 GPS      -> ESP32-C3 SuperMini via GPS UART/NMEA
Cat-M module    -> ESP32-C3 SuperMini via LTE UART/AT commands
ESP32-C3        -> active buzzer and standalone trigger
Battery         -> powers prototype through tested power path
```

