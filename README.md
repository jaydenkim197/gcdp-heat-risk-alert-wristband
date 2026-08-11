# GCDP Heat Risk Alert Wristband

Wearable safety-alert prototype developed for the 2025-2 Global CDP
Thailand-Korea program. The device measures pulse and skin temperature,
acquires GNSS coordinates, and sends an emergency SMS over LTE-M.

![Modular housing CAD](media/cad/260805_chain_housing_cad_isometric.png)

## Prototype Hardware

- ESP32-C3 SuperMini controller
- M5Stack Stamp CatM S003 (SIM7080G) LTE-M/NB-IoT modem
- Beitian BE-220 GNSS module
- MAX30102 pulse/PPG sensor
- TMP117 temperature sensor
- 3 V active buzzer
- 3.7 V 220 mAh Li-Po battery
- Modular 3D-printed hinged housings

## Verified Prototype Functions

- Concurrent MAX30102 and TMP117 acquisition over I2C
- LTE network registration with an activated SKT-network SIM
- GNSS coordinate acquisition
- SMS transmission containing heart rate, temperature, and location
- Battery-powered operation of the integrated prototype
- Standalone buzzer-guided measurement workflow

## ESP32-C3 SuperMini Pin Map

| Function | GPIO |
| --- | ---: |
| I2C SDA (MAX30102/TMP117) | 8 |
| I2C SCL (MAX30102/TMP117) | 9 |
| Stamp CatM RX into ESP | 20 |
| Stamp CatM TX from ESP | 21 |
| BE-220 RX into ESP | 6 |
| BE-220 TX from ESP | 7 |
| Active buzzer | 10 |
| Test trigger | 4 |
| Battery-test input | 5 |

UART labels above are from the ESP32 perspective. Cross the physical UART
connections: module TX to ESP RX, and module RX to ESP TX. All modules must
share ground.

## Repository Layout

- `firmware/`: PlatformIO firmware and captured sensor/UART evidence
- `hardware/pcb/`: Altium schematic and PCB design history
- `hardware/enclosure/`: Fusion export, STEP/STL files, generators, and fit notes
- `docs/`: dated bring-up, integration, power, and module-selection records
- `media/cad/`: selected enclosure renders for quick review

Third-party datasheets, downloaded CAD models, raw presentation media,
procurement files, personal identifiers, and generated build caches are not
tracked in this repository.

## Build and Upload

Open `firmware/` as a PlatformIO project, then run:

```powershell
pio run -e esp32c3_supermini
pio run -e esp32c3_supermini -t upload
pio device monitor -b 115200
```

Before field use, replace the placeholder `STANDALONE_ALERT_NUMBER` in
`firmware/include/pins.h` with the intended recipient number.

## Status

This repository documents a functional presentation prototype. It is not a
certified medical device, and measured values must not be used for diagnosis
or treatment decisions.
