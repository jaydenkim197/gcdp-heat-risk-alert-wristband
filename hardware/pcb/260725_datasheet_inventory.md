# Datasheet / PCB Reference Inventory - 2026-07-25

Purpose: local inventory of electrical and PCB reference documents for the GCDP heat-risk alert wristband.

## Datasheets

| Part | Local file | Status | Use |
| --- | --- | --- | --- |
| ESP32-C3 chip | `esp32_c3_chip_datasheet_en.pdf` | Stored | Chip-level electrical reference. Use for PCB design only if moving beyond SuperMini carrier. |
| M5Stack Stamp CatM S003 | `m5stack_stamp_catm_s003.pdf` | Stored | Stamp CatM module pinout/spec reference. Primary LTE module document for the current architecture. |
| SIM7080 series | `sim7080_series_spec_20200427.pdf` | Stored | SIM7080G chip/module electrical reference. Use for power, UART, SIM, RF, and PCB design decisions. |

## PCB References

| Part | Local file | Status | Use |
| --- | --- | --- | --- |
| Adafruit TMP117 PCB | `..\reference_pcbs\tmp117_adafruit\adafruit_tmp117_pcb_main.zip` | Stored | Official Adafruit PCB source archive. |
| Adafruit TMP117 PCB folder | `..\reference_pcbs\tmp117_adafruit\Adafruit-TMP117-PCB-main\Adafruit_TMP117.brd` | Moved from Downloads | Eagle board reference. |
| Adafruit TMP117 schematic folder | `..\reference_pcbs\tmp117_adafruit\Adafruit-TMP117-PCB-main\Adafruit_TMP117.sch` | Moved from Downloads | Eagle schematic reference. |

## Case / Mechanical Datasheets

These are stored under `06_3d_case/references/datasheets` because they are mostly useful for enclosure/blockout dimensions:

| Part | Local file | Use |
| --- | --- | --- |
| Adafruit TMP117 breakout | `..\..\06_3d_case\references\datasheets\adafruit_tmp117_high_accuracy_i2c_temperature_monitor.pdf` | Mechanical/pin reference for TMP117 breakout. |
| ESP32-C3 SuperMini board | `..\..\06_3d_case\references\datasheets\esp32_c3_supermini_board_datasheet.pdf` | Board-level mechanical/pin reference for SuperMini carrier. |

## Duplicate / Not Copied

| Source file | Reason |
| --- | --- |
| `C:\Users\sukhe\Downloads\Adafruit TMP117 (1).fzpz` | Duplicate of `Adafruit TMP117.fzpz` by SHA256 hash. Only one copy was stored. |

## Source Cleanup

The listed downloaded source files and folder were moved out of `C:\Users\sukhe\Downloads` into this workspace on 2026-07-25. The Downloads copies are no longer present.
