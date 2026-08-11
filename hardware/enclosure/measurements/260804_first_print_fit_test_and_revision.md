# 260804 First Print Fit Test And Revision

Date: 2026-08-04

Status: first physical print inspected; geometry corrections required before
wiring-channel authoring

## 1. Gate Result

The first enclosure print is not the wiring-layout baseline yet.

```text
LTE housing:     conditional fail (module fit)
ESP/battery:     conditional pass (USB-C position and buzzer simplification)
GPS housing:     pass for module volume; connector clearance missing
MAX30102 housing: conditional fail (slide fit and supported entrance)
TMP117 housing:  fail (slide fit, support residue, and closure fit)
```

Wiring slots and hinge-crossing channels shall be added only after the changes
below are applied. Otherwise, later fit corrections may erase or misalign the
cable features.

## 2. LTE Housing Revision

Observed facts:

- A nominal 20 x 30 mm module fit is too tight.

Required changes:

1. Make the finished LTE module pocket at least 20.6 x 30.8 mm in plan.
2. If the current print already provides 20.8 mm across the 20 mm module axis,
   retain that width and change only the 30 mm axis.
3. The finished pocket length after printing must be at least 30.6 mm. Do not
   use the observed 28.5 mm result as the next target.
4. Keep the current overall housing height unless a real component collision is
   found; the first print did not identify insufficient vertical room.

## 3. ESP32-C3 And Battery Housing Revision

Observed facts:

- The stacked physical assembly was recorded as 20.8 x 28.5 x 12.9 mm.
- The USB-C opening is vertically misaligned.
- The existing free volume is sufficient for the buzzer; a dedicated buzzer
  compartment is unnecessary.
- The previously reserved buzzer region was approximately 9 x 11 mm.

Required changes:

1. Lower the USB-C cutout by exactly 1.85 mm from its current center position.
2. Remove the dedicated 9 x 11 mm buzzer partition or close-fitting pocket.
3. Retain an unobstructed 9 x 11 mm placement zone and a sound outlet.
4. Do not reduce the 20.8 x 28.5 x 12.9 mm proven assembly envelope during
   this revision.

## 4. GPS Housing Revision

Observed facts:

- The module volume fits.
- The current physical measurement was recorded as 20.7 x 23.7 x 5.3 mm.
- The connector and wire exit do not have a dedicated service volume.

Required changes:

1. Do not enlarge the complete GPS pocket solely for tolerance; its fit passed.
2. Add a connector bay on the connector edge. Initial minimum envelope:
   8 mm along the cable direction, 8 mm across, and 4 mm high.
3. Add a rounded wire exit of approximately 5.0 x 2.5 mm.
4. Add at least R1.0 mm to the wire-contacting slot edges.
5. Keep wiring below the ceramic patch perimeter and never route across its
   outward-facing antenna surface.

The 8 x 8 x 4 mm connector bay is a first-print allowance and must be checked
against the actual terminated connector before the final print.

## 5. MAX30102 Housing Revision

Observed printed slide space:

```text
width:  21.0 mm
height: 1.05 mm
depth:  19.8 mm
```

Observed behavior:

- The board enters but is extremely tight and does not reach the final stop.
- Support removal narrows and roughens the entrance.
- The 2 mm skin-side floor is thicker than required.

Required changes:

1. Increase slide width from 21.0 to 21.5 mm.
2. Increase rail-slot height from 1.05 to 1.55 mm.
3. Do not increase the full 19.8 mm depth by default. First remove entrance and
   rail friction; depth was not proven to be the limiting dimension.
4. Add a 0.6 mm x 45 degree lead-in chamfer at both rail entrances.
5. Add a 0.3 mm elephant-foot relief on bed-facing slot edges.
6. Redesign the slot roof as a self-supporting 45 degree profile so slicer
   support is not generated inside the slide rails.
7. Reduce the skin-side floor from 2.0 to 1.2 mm, while retaining at least
   2.0 mm around the outer frame, hinge, and load-bearing edges.
8. Preserve the optical opening and light-blocking perimeter. The optical
   package must remain the skin-contact high point.

## 6. TMP117 Housing Revision

Observed printed slide/closure dimensions:

```text
width:  18.15 mm
height: 1.60 mm
depth:  29.8 mm
```

Observed behavior:

- The board enters tightly but does not reach the final stop.
- Support removal affects the slide path.
- The closure/lid cannot enter at all.
- The 2 mm skin-side floor is thicker than required.

Required changes:

1. Increase slide width from 18.15 to 18.50 mm.
2. Increase rail-slot height from 1.60 to 1.95 mm.
3. Retain the current 29.8 mm depth unless inspection finds a physical rear
   collision; depth is already greater than the nominal board length.
4. Add a 0.6 mm x 45 degree lead-in chamfer and 0.3 mm elephant-foot relief.
5. Use a self-supporting 45 degree slot roof and block slicer support inside the
   rail path.
6. Reduce the skin-side floor from 2.0 to 1.2 mm, retaining a 2.0 mm perimeter
   frame and the current reinforced hinge geometry.
7. Give the closure tongue 0.6 mm total planar clearance relative to its mating
   opening, equivalent to 0.3 mm per side.
8. Give the closure insertion thickness at least 0.35 mm clearance and add a
   0.5 mm x 45 degree lead-in chamfer.
9. Keep the central thermal-contact region free of wire bundles and thick ribs.

## 7. Printability Rules For Both Sensor Housings

- No generated support is permitted inside a functional slide rail.
- Print the largest closed outer face on the bed when practical.
- Use 45 degree roofs or short bridges over narrow slots.
- Use 0.3 mm elephant-foot relief on all insertion openings touching the bed.
- Keep the sensor-contact membrane at 1.2 mm and the structural perimeter at
  2.0 mm or more.
- Fit must be verified without sanding the PCB before final enclosure printing.
- Print only fit coupons or the two sensor housings after this revision. A full
  five-section reprint is unnecessary at this gate.

## 8. Revision Order

```text
1. LTE pocket length and USB-C position
2. ESP/battery USB-C position and buzzer partition removal
3. GPS connector bay and wire exit
4. MAX30102 slide clearance and support-free entrance
5. TMP117 slide and closure clearance
6. Print fit coupons / sensor housings
7. Confirm full insertion and closure by hand
8. Freeze module pockets
9. Author wiring channels and hinge free-loop clearances
```

## 9. Milestone Status

```text
Mechanical architecture selected: complete
Hinge concept and first hinge print: complete
First enclosure fit print: complete
Fit-correction geometry: in progress
Fit-correction print verification: pending
Wiring-channel CAD: blocked by fit-correction gate
Final enclosure print: pending
```
