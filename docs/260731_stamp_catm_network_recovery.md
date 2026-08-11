# 2026-07-31 Stamp CatM Network Recovery

## Purpose

Resolve the M5Stack Stamp CatM registration failure after UART, power, SIM
detection, antenna reception, and operator scanning had already passed.

## Reference Findings

The Korean Cat-M1 registration note at:

```text
https://ilikethisplus.tistory.com/65
```

distinguishes the general SKT LTE PLMN `45005` from the SKT Cat-M1 PLMN
`45012`. It also notes that `CPIN READY` alone does not prove that a line is
currently provisioned and that `CEREG stat 3` means registration denied.

Other SIM7080 and Stamp CatM reports consistently identify these checks:

```text
UART: 115200 baud, 8N1
LTE-only mode: AT+CNMP=38
Cat-M-only preference: AT+CMNB=1
Korean Cat-M bands: B1/B3/B5 are relevant; Korean examples also use B8
Clear stored registration/RPLMN data after a SIM or network change
Verify PDP/APN configuration before data activation
```

M5Stack Stamp CatM users also reported that correct UART pin mapping and a
stable 5 V supply are prerequisites. Both had already been verified on this
device.

## Live Baseline Readback

The Stamp was queried through ESP32-C3 USB serial on COM7.

```text
AT: OK
CFUN: 1
CNMP: 2
CMNB: 1
Cat-M bands: all module-supported bands
Carrier profile: automatic selection, Default active
CSDP: 2
Cell lock: disabled
COPS: automatic
CEREG: 0,2
CPSI: NO SERVICE
CGATT: 0
PDP context 1: IP, APN "YourAPN"
PDP context 2: IPV4V6, APN "ims"
```

Two local configuration discrepancies were found:

1. The live Stamp value was `CNMP=2` automatic, not the previously recorded
   `CNMP=38` LTE-only value.
2. PDP context 1 contained the literal example placeholder `YourAPN`.

## Recovery Commands

RF was stopped before clearing saved registration information.

```text
AT+CFUN=0
AT+CNMP=38
AT+CMNB=1
AT+CBANDCFG="CAT-M",1,3,5
AT+CGDCONT=1,"IP",""
AT+CLRNET=1,1
AT+CMCFG=1
AT+CFUN=1,1
```

Notes:

- `AT+CGDCONT=1` alone returned `ERROR`; explicitly setting an empty APN
  returned `OK`.
- PDP context 2, `ims`, was not modified.
- `AT+CLRNET=1,1` cleared Cat-M registration information and the last
  registered PLMN radio-access technology.
- The sequence did not erase the SIM, ICCID, IMSI, IMEI, or SMS records.

## Result

After modem reboot and automatic network search:

```text
AT+CPIN?: READY
AT+CNMP?: 38
AT+CMNB?: 1
AT+CBANDCFG?: CAT-M B1/B3/B5
AT+COPS?: automatic, SKTelecom, LTE-M access technology
AT+CEREG?: 0,1
AT+CPSI?: LTE CAT-M1, Online, 450-05, EUTRAN Band 5
AT+CGATT?: 1
AT+CSQ: 24,99
```

Final verdict:

```text
Stamp hardware: PASS
SIM detection: PASS
SKT Cat-M registration: PASS
Packet attachment: PASS
Korean network compatibility: CONFIRMED
Carrier intervention: NOT REQUIRED FOR THIS RECOVERY
SMS transmission: PASS
```

Because several stored settings were corrected as one controlled recovery
sequence, this test does not isolate one single command as the sole cause. The
combined local configuration cleanup resolved the failure without changing the
SIM, antenna, power source, UART wiring, or carrier account.

## Firmware Action

The verified initialization policy should be implemented in firmware:

```text
Set CNMP=38
Set CMNB=1
Use Cat-M bands B1/B3/B5 for the Korean prototype
Do not ship with placeholder APN "YourAPN"
Use the actual provider APN only when packet data is required
Do not run CLRNET on every boot; reserve it for recovery or SIM changes
```

The firmware was updated to apply the non-destructive settings on LTE
initialization and before the standalone alert registration step:

```text
AT+CNMP=38
AT+CMNB=1
AT+CBANDCFG="CAT-M",1,3,5
```

`CLRNET` and APN changes were intentionally not added to every boot.

Verification:

```text
PlatformIO environment: esp32c3_supermini
Build: PASS
Upload through COM7: PASS
Post-upload LTE status:
  SIM READY
  CEREG 0,1
  COPS SKTelecom, AcT 7
  CGATT 1
  CPSI LTE CAT-M1, Online, 450-05, Band 5
  CSQ 22 to 25
```

## Controlled SMS Test

The registered Stamp sent SMS messages to the previously verified project test
number. The phone number was passed as digits only, without hyphens.

Received messages:

```text
Stamp CatM LTE-M test OK
Stamp CatM test OK
```

Two messages were received. The first host-side diagnostic process was stopped
after the ESP32 had already accepted the SMS command; stopping the PC process
did not stop the ESP32 or modem, so that first message completed in the
background. A second explicit test then completed as well. This explains the
two messages and is not a modem duplicate-send fault.

No additional SMS should be sent during this diagnostic session.

## Milestone After Recovery

```text
M1 Sensor bring-up with ESP32-C3: COMPLETE
M2 Previous SIM7080 integrated proof of concept: COMPLETE
M3 Stamp CatM power, UART, SIM, RF, registration: COMPLETE
M4 Stamp CatM SMS transmission: COMPLETE
M5 External BE-220 GPS UART and position fix: NEXT
M6 Sensor + BE-220 + Stamp alert payload: PENDING
M7 Stamp-based battery-only full-system test: PENDING
M8 Soldered wearable prototype layout: PENDING
M9 Prototype enclosure and wrist mounting: PENDING
M10 Custom PCB: AFTER WEARABLE PROTOTYPE
```

## BE-220 Preparation For Next Milestone

The ESP32-C3 SuperMini firmware now reserves a second UART for the external
BE-220 GPS module:

```text
BE-220 TX -> ESP32-C3 GPIO6 (GPS RX)
BE-220 RX -> ESP32-C3 GPIO7 (GPS TX; optional for first read-only test)
BE-220 GND -> common GND
BE-220 VCC -> 5 V
UART: 38400 baud, 8N1
```

TinyGPSPlus 1.1.0 was added for NMEA parsing. The `GPS` command waits for a
valid external fix and reports latitude, longitude, received byte count,
satellite count, HDOP, and checksum failures. `GPSOFF` releases the GPS UART;
it does not switch off the separately powered BE-220.

The SuperMini firmware build and COM7 upload passed. Physical BE-220 UART and
position-fix validation remains pending until the module is connected.

## Sources

- Korean Cat-M1 registration process:
  https://ilikethisplus.tistory.com/65
- Korean SIM7000/SIM7080 usage example:
  https://shga.kr/archives/648
- M5Stack Stamp CatM user report:
  https://community.m5stack.com/topic/5107/stamp-catm-does-not-respond-solved-html-get-and-send-email
- SIM7070/SIM7080/SIM7090 AT command manual:
  https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/unit/sim7080g/en/SIM7070_SIM7080_SIM7090%20Series_AT%20Command%20Manual_V1.04.pdf
