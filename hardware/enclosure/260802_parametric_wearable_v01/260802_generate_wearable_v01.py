from __future__ import annotations

import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import trimesh

from build123d import (
    Align,
    Axis,
    Box,
    BuildPart,
    BuildSketch,
    Compound,
    Ellipse,
    Location,
    Mode,
    Plane,
    Shape,
    export_step,
    export_stl,
    extrude,
)


ROOT = Path(__file__).resolve().parent
EXPORTS = ROOT / "exports"
EXPORTS.mkdir(parents=True, exist_ok=True)

# Wrist and strap reference. The 59 x 46 mm ellipse has a Ramanujan
# circumference of about 165.6 mm. It is a fit reference, not a rigid cuff.
WRIST_WIDTH = 59.0
WRIST_DEPTH = 46.0
STRAP_WIDTH = 25.0
STRAP_THICKNESS = 2.0
STRAP_SLOT_HEIGHT = STRAP_THICKNESS + 0.6
STRAP_CLEARANCE = 0.6

# FDM prototype defaults.
WALL = 1.6
BOTTOM = 1.6
LID = 1.4
CORNER = 6.0
FIT_XY = 0.5
FIT_Z = 0.4


def ellipse_circumference(width: float, depth: float) -> float:
    a = width / 2.0
    b = depth / 2.0
    h = ((a - b) / (a + b)) ** 2
    return math.pi * (a + b) * (1 + 3 * h / (10 + math.sqrt(4 - 3 * h)))


def rounded_box(x: float, y: float, z: float, radius: float = CORNER) -> Shape:
    body = Box(x, y, z, align=(Align.CENTER, Align.CENTER, Align.MIN))
    return body.fillet(min(radius, x / 4, y / 4), body.edges().filter_by(Axis.Z))


def enclosure_parts(
    name: str,
    inner_x: float,
    inner_y: float,
    inner_z: float,
) -> tuple[Shape, Shape, dict[str, float]]:
    outer_x = inner_x + 2 * WALL
    outer_y = inner_y + 2 * WALL
    base_z = BOTTOM + inner_z

    outer = rounded_box(outer_x, outer_y, base_z)
    cavity = Box(
        inner_x,
        inner_y,
        inner_z + 0.2,
        align=(Align.CENTER, Align.CENTER, Align.MIN),
    ).translate((0, 0, BOTTOM))
    base = outer - cavity

    # Low-profile side rails retain the continuous strap under the pod. Unlike
    # a closed tunnel, this only adds the strap clearance to the total height.
    tunnel_outer_y = STRAP_WIDTH + 2 * WALL + STRAP_CLEARANCE
    rail = Box(
        outer_x,
        WALL,
        STRAP_SLOT_HEIGHT,
        align=(Align.CENTER, Align.CENTER, Align.MIN),
    )
    rail_y = (STRAP_WIDTH + STRAP_CLEARANCE + WALL) / 2
    rails = rail.translate((0, rail_y, 0)) + rail.translate((0, -rail_y, 0))
    base = base.translate((0, 0, STRAP_SLOT_HEIGHT)) + rails

    lid = rounded_box(outer_x, outer_y, LID, 5.5)

    dims = {
        "name": name,
        "outer_x": outer_x,
        "outer_y": max(outer_y, tunnel_outer_y),
        "outer_z": base_z + STRAP_SLOT_HEIGHT + LID,
        "inner_x": inner_x,
        "inner_y": inner_y,
        "inner_z": inner_z,
    }
    return base, lid, dims


def wrist_reference() -> Shape:
    a = WRIST_WIDTH / 2.0
    b = WRIST_DEPTH / 2.0
    with BuildPart() as ring:
        with BuildSketch(Plane.XZ):
            Ellipse(a + STRAP_THICKNESS, b + STRAP_THICKNESS)
            Ellipse(a, b, mode=Mode.SUBTRACT)
        extrude(amount=STRAP_WIDTH / 2, both=True)
    return ring.part


def place_on_wrist(shape: Shape, angle_deg: float, radial_offset: float) -> Shape:
    theta = math.radians(angle_deg)
    a = WRIST_WIDTH / 2.0
    b = WRIST_DEPTH / 2.0
    x = a * math.cos(theta)
    z = b * math.sin(theta)

    # Ellipse surface normal, normalized. Local +Z is rotated onto this normal.
    nx = math.cos(theta) / a
    nz = math.sin(theta) / b
    norm = math.hypot(nx, nz)
    nx /= norm
    nz /= norm
    x += nx * radial_offset
    z += nz * radial_offset
    rotation_y = math.degrees(math.atan2(nx, nz))
    return Location((x, 0, z), (0, rotation_y, 0)) * shape


def export_shape(shape: Shape, stem: str, stl: bool = True) -> None:
    export_step(shape, EXPORTS / f"{stem}.step")
    if stl:
        export_stl(shape, EXPORTS / f"{stem}.stl", tolerance=0.05, angular_tolerance=0.1)


def render_preview(stem: str, elevation: float = 22, azimuth: float = -58) -> None:
    loaded = trimesh.load(EXPORTS / f"{stem}.stl")
    if isinstance(loaded, trimesh.Scene):
        mesh = trimesh.util.concatenate(tuple(loaded.geometry.values()))
    else:
        mesh = loaded

    vertices = np.asarray(mesh.vertices)
    faces = np.asarray(mesh.faces)
    fig = plt.figure(figsize=(10, 8), dpi=160)
    ax = fig.add_subplot(111, projection="3d")
    ax.plot_trisurf(
        vertices[:, 0],
        vertices[:, 1],
        vertices[:, 2],
        triangles=faces,
        color="#6f87ad",
        edgecolor="#26364f",
        linewidth=0.08,
        antialiased=True,
        shade=True,
    )
    mins = vertices.min(axis=0)
    maxs = vertices.max(axis=0)
    center = (mins + maxs) / 2
    span = float((maxs - mins).max()) / 2
    ax.set_xlim(center[0] - span, center[0] + span)
    ax.set_ylim(center[1] - span, center[1] + span)
    ax.set_zlim(center[2] - span, center[2] + span)
    ax.set_box_aspect((1, 1, 1))
    ax.view_init(elev=elevation, azim=azimuth)
    ax.set_axis_off()
    fig.tight_layout(pad=0)
    fig.savefig(ROOT / f"{stem}_preview.png", transparent=False, bbox_inches="tight")
    plt.close(fig)


def main() -> None:
    # Internal component envelopes plus assembly allowance.
    # 36.8 mm internal length reserves a separate 10.2 mm end zone for the
    # measured 9.1 x 5.9 mm cylindrical buzzer without increasing stack height.
    battery_esp = enclosure_parts("battery_esp_buzzer", 36.8, 21.6, 10.5)
    gps = enclosure_parts("gps", 23.2, 21.2, 7.3)
    stamp = enclosure_parts("stamp_catm", 31.5, 21.2, 7.0)

    sensor_bar = rounded_box(51.0, 21.5, 8.0, 2.8)
    lte_antenna = Box(
        50.0,
        25.0,
        0.13,
        align=(Align.CENTER, Align.CENTER, Align.MIN),
    )

    ring = wrist_reference()
    export_shape(ring, "260802_wrist_165mm_strap_25mm_reference")

    def closed_pod(parts: tuple[Shape, Shape, dict[str, float]]) -> Shape:
        base, lid, dims = parts
        lid_z = dims["outer_z"] - LID
        return base + lid.translate((0, 0, lid_z))

    # Three-pod layout: thinner and more conformal than combining GPS + Stamp.
    three_pod = Compound.make_composite(
        [
            ring,
            place_on_wrist(closed_pod(battery_esp), 132, 8.0),
            place_on_wrist(closed_pod(gps), 90, 6.0),
            place_on_wrist(closed_pod(stamp), 48, 7.0),
            place_on_wrist(sensor_bar, 270, 5.0),
            place_on_wrist(lte_antenna, 12, 2.5),
        ]
    )
    export_shape(three_pod, "260802_layout_3pod_v01")

    # Two-pod comparison only. GPS and Stamp are shown side-by-side because
    # stacking them would create a roughly 14 mm internal stack.
    gps_stamp_combo = rounded_box(56.0, 24.5, 10.0, 2.5)
    two_pod = Compound.make_composite(
        [
            ring,
            place_on_wrist(closed_pod(battery_esp), 125, 8.0),
            place_on_wrist(gps_stamp_combo, 65, 7.0),
            place_on_wrist(sensor_bar, 270, 5.0),
            place_on_wrist(lte_antenna, 12, 2.5),
        ]
    )
    export_shape(two_pod, "260802_layout_2pod_comparison_v01")

    render_preview("260802_layout_3pod_v01")
    render_preview("260802_layout_2pod_comparison_v01")

    for stem, parts in [
        ("battery_esp", battery_esp),
        ("gps", gps),
        ("stamp_catm", stamp),
    ]:
        base, lid, _ = parts
        export_shape(base, f"260802_{stem}_pod_base_v01")
        export_shape(lid, f"260802_{stem}_pod_lid_v01")

    export_shape(sensor_bar, "260802_sensor_bar_envelope_v01")

    print(f"Wrist reference circumference: {ellipse_circumference(WRIST_WIDTH, WRIST_DEPTH):.2f} mm")
    for _, _, dims in [battery_esp, gps, stamp]:
        print(
            f"{dims['name']}: "
            f"{dims['outer_x']:.1f} x {dims['outer_y']:.1f} x {dims['outer_z']:.1f} mm"
        )


if __name__ == "__main__":
    main()
