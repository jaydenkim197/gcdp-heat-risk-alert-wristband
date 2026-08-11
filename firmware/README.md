# Heat Risk Alert Wristband Firmware

Initial PlatformIO firmware workspace for the GCDP Korea prototype.

## Hardware Context

Target device:
- ESP32 main controller
- MAX30102 PPG sensor on I2C
- TMP117 temperature sensor on I2C
- SIM7080G LTE/GNSS module on UART
- Active buzzer on GPIO
- Li-Po battery with TP4056 charging and power regulation

## First Bring-Up Order

1. Build and upload the `esp32dev` environment first.
2. Open the serial monitor at `115200`.
3. Confirm the buzzer chirps once on boot.
4. Confirm I2C scan sees:
   - `0x48` for TMP117
   - `0x57` for MAX30102
5. Confirm the LTE module responds to `AT`.
6. Only after these pass, move to SMS/GPS and state-machine integration.

## Common Commands

From this directory:

```powershell
$pio = "$env:APPDATA\Python\Python311\Scripts\pio.exe"
& $pio run -e esp32dev
& $pio run -e esp32dev -t upload
& $pio device monitor -b 115200
```

Other configured environments:

```powershell
& $pio run -e xiao_esp32c3
& $pio run -e esp32c3_supermini
```

## Default Pin Map

ESP32 DevKitC:

| Function | GPIO |
| --- | ---: |
| I2C SDA | 21 |
| I2C SCL | 22 |
| LTE RX | 16 |
| LTE TX | 17 |
| Buzzer | 25 |

Seeed XIAO ESP32-C3:

| Function | GPIO |
| --- | ---: |
| I2C SDA | 6 |
| I2C SCL | 7 |
| LTE RX | 20 |
| LTE TX | 21 |
| Buzzer | 10 |

ESP32-C3 SuperMini defaults are provisional. Verify against the exact board pinout before wiring.

## Notes

- LTE modules can draw short current spikes above 1 A. Use a stable supply and place bulk capacitance close to the LTE module power input.
- Keep ESP32 and SIM7080G UART grounds common.
- For SIM7080G UART, connect ESP32 TX to module RX and ESP32 RX to module TX.
- The first firmware is intentionally a bring-up sketch, not the final emergency algorithm.
