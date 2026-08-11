# GCDP HRA Wristband Prototype

Current status as of 2026-07-20: this folder is no longer the main PCB design source. The active prototype direction is a wired wearable using fixed modules.

## Current Prototype Direction

```text
ESP32-C3 SuperMini
  + M5Stack Stamp CatM S003 LTE/SMS module
  + Beitian BE-220 GPS module
  + TMP117 module
  + MAX30102 module
  + active buzzer
  + TW-602035 Li-Po battery
  + TPU printed wristband/holder
```

## Fixed Components

See:

```text
C:\Users\sukhe\Desktop\GCDP_codex_temp\01_docs\260720_fixed_parts.md
C:\Users\sukhe\Desktop\GCDP_codex_temp\01_docs\260720_fixed_parts_and_architecture.md
```

## Notes

- Keep this folder only as a reference for previous PCB planning.
- Do not treat old RevA PCB notes as the current build plan unless they match the fixed parts document.
- The large SIM7080G developer board remains a validation/reference platform only.
- BE-220 is now the selected GPS module.
- LTE/SMS is handled by the small Cat-M module.
- The case must prioritize sensor skin contact, GPS outward placement, LTE antenna clearance, and wire strain relief.
