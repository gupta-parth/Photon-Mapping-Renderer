#!/usr/bin/env python3
"""Generate and validate the Crystal Optics Gallery OBJ and MTL assets."""

from __future__ import annotations

import math
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


Vec3 = tuple[float, float, float]
Corner = tuple[int, int]
Face = tuple[Corner, Corner, Corner]


def add(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a: Vec3, scale: float) -> Vec3:
    return (a[0] * scale, a[1] * scale, a[2] * scale)


def dot(a: Vec3, b: Vec3) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a: Vec3, b: Vec3) -> Vec3:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def length(a: Vec3) -> float:
    return math.sqrt(dot(a, a))


def normalize(a: Vec3) -> Vec3:
    norm = length(a)
    if norm <= 1.0e-12:
        raise ValueError("cannot normalize a zero-length vector")
    return mul(a, 1.0 / norm)


def mean(points: Iterable[Vec3]) -> Vec3:
    values = list(points)
    count = float(len(values))
    return (
        sum(p[0] for p in values) / count,
        sum(p[1] for p in values) / count,
        sum(p[2] for p in values) / count,
    )


def rotate_lens(v: Vec3) -> Vec3:
    """Tilt around local X, then world Z, preserving scale."""
    rx = math.radians(26.0)
    rz = math.radians(-15.0)
    x0, y0, z0 = v
    x1 = x0
    y1 = math.cos(rx) * y0 - math.sin(rx) * z0
    z1 = math.sin(rx) * y0 + math.cos(rx) * z0
    return (
        math.cos(rz) * x1 - math.sin(rz) * y1,
        math.sin(rz) * x1 + math.cos(rz) * y1,
        z1,
    )


@dataclass
class Mesh:
    name: str
    material: str
    positions: list[Vec3] = field(default_factory=list)
    normals: list[Vec3] = field(default_factory=list)
    faces: list[Face] = field(default_factory=list)

    def smooth_face(self, corners: list[Corner]) -> None:
        p0, p1, p2 = (self.positions[v] for v, _ in corners)
        expected = mean(self.normals[n] for _, n in corners)
        geometric = cross(sub(p1, p0), sub(p2, p0))
        if dot(geometric, expected) < 0.0:
            corners[1], corners[2] = corners[2], corners[1]
        self.faces.append((corners[0], corners[1], corners[2]))

    def flat_face(self, vertices: list[int], interior: Vec3) -> None:
        p0, p1, p2 = (self.positions[v] for v in vertices)
        geometric = cross(sub(p1, p0), sub(p2, p0))
        centroid = mean((p0, p1, p2))
        if dot(geometric, sub(centroid, interior)) < 0.0:
            vertices[1], vertices[2] = vertices[2], vertices[1]
            p1, p2 = p2, p1
            geometric = cross(sub(p1, p0), sub(p2, p0))
        normal_index = len(self.normals)
        self.normals.append(normalize(geometric))
        self.faces.append(tuple((v, normal_index) for v in vertices))


def merge(target: Mesh, source: Mesh) -> None:
    vertex_offset = len(target.positions)
    normal_offset = len(target.normals)
    target.positions.extend(source.positions)
    target.normals.extend(source.normals)
    for face in source.faces:
        target.faces.append(
            tuple(
                (vertex_offset + vertex, normal_offset + normal)
                for vertex, normal in face
            )
        )


def make_quad(name: str, material: str, points: list[Vec3],
              desired_normal: Vec3) -> Mesh:
    mesh = Mesh(name, material, positions=points)
    normal = normalize(desired_normal)
    mesh.normals.append(normal)
    candidates = ([0, 1, 2], [0, 2, 3])
    for vertices in candidates:
        p0, p1, p2 = (mesh.positions[v] for v in vertices)
        if dot(cross(sub(p1, p0), sub(p2, p0)), normal) < 0.0:
            vertices[1], vertices[2] = vertices[2], vertices[1]
        mesh.faces.append(tuple((vertex, 0) for vertex in vertices))
    return mesh


def make_box(name: str, material: str, low: Vec3, high: Vec3) -> Mesh:
    x0, y0, z0 = low
    x1, y1, z1 = high
    positions = [
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    ]
    mesh = Mesh(name, material, positions=positions)
    center = mean(positions)
    quads = (
        (0, 3, 2, 1), (4, 5, 6, 7),
        (0, 4, 7, 3), (1, 2, 6, 5),
        (0, 1, 5, 4), (3, 7, 6, 2),
    )
    for a, b, c, d in quads:
        mesh.flat_face([a, b, c], center)
        mesh.flat_face([a, c, d], center)
    return mesh


def make_gallery() -> list[Mesh]:
    x0, x1 = -2.1, 2.1
    y0, y1 = 0.0, 2.65
    z0, z1 = -1.85, 1.45
    return [
        make_quad(
            "gallery_floor", "ivory_floor",
            [(x0, y0, z0), (x1, y0, z0),
             (x1, y0, z1), (x0, y0, z1)],
            (0.0, 1.0, 0.0),
        ),
        make_quad(
            "gallery_ceiling", "ivory_ceiling",
            [(x0, y1, z1), (x1, y1, z1),
             (x1, y1, z0), (x0, y1, z0)],
            (0.0, -1.0, 0.0),
        ),
        make_quad(
            "gallery_back", "ivory_back",
            [(x0, y0, z0), (x0, y1, z0),
             (x1, y1, z0), (x1, y0, z0)],
            (0.0, 0.0, 1.0),
        ),
        make_quad(
            "gallery_left", "burgundy_wall",
            [(x0, y0, z1), (x0, y1, z1),
             (x0, y1, z0), (x0, y0, z0)],
            (1.0, 0.0, 0.0),
        ),
        make_quad(
            "gallery_right", "blue_wall",
            [(x1, y0, z0), (x1, y1, z0),
             (x1, y1, z1), (x1, y0, z1)],
            (-1.0, 0.0, 0.0),
        ),
    ]


def make_trefoil() -> Mesh:
    mesh = Mesh("glass_trefoil_knot", "glass_knot")
    path_segments = 120
    ring_segments = 12
    major_radius = 0.50
    knot_radius = 0.15
    tube_radius = 0.070
    center = (0.0, 1.32, -0.20)

    for i in range(path_segments):
        t = 2.0 * math.pi * i / path_segments
        q = major_radius + knot_radius * math.cos(3.0 * t)
        dq = -3.0 * knot_radius * math.sin(3.0 * t)
        point = (
            q * math.cos(2.0 * t),
            q * math.sin(2.0 * t),
            knot_radius * math.sin(3.0 * t),
        )
        tangent = normalize((
            dq * math.cos(2.0 * t) - 2.0 * q * math.sin(2.0 * t),
            dq * math.sin(2.0 * t) + 2.0 * q * math.cos(2.0 * t),
            3.0 * knot_radius * math.cos(3.0 * t),
        ))
        frame_normal = normalize(cross((0.0, 0.0, 1.0), tangent))
        frame_binormal = normalize(cross(tangent, frame_normal))

        for j in range(ring_segments):
            angle = 2.0 * math.pi * j / ring_segments
            radial = add(
                mul(frame_normal, math.cos(angle)),
                mul(frame_binormal, math.sin(angle)),
            )
            mesh.positions.append(add(center, add(point, mul(radial, tube_radius))))
            mesh.normals.append(normalize(radial))

    def index(i: int, j: int) -> int:
        return (i % path_segments) * ring_segments + (j % ring_segments)

    for i in range(path_segments):
        for j in range(ring_segments):
            a = index(i, j)
            b = index(i + 1, j)
            c = index(i + 1, j + 1)
            d = index(i, j + 1)
            mesh.smooth_face([(a, a), (b, b), (c, c)])
            mesh.smooth_face([(a, a), (c, c), (d, d)])
    return mesh


def make_lens() -> Mesh:
    mesh = Mesh("glass_biconvex_lens", "glass_lens")
    segments = 48
    radial_steps = 5
    radius = 0.33
    # The curvature focuses near the floor at eta=1.5.
    half_thickness = 0.08
    center = (-1.19, 0.78, -0.20)

    def position(radial: float, angle: float, upper: bool) -> Vec3:
        x = radial * math.cos(angle)
        z = radial * math.sin(angle)
        sign = 1.0 if upper else -1.0
        y = sign * half_thickness * (1.0 - (radial / radius) ** 2)
        return add(center, rotate_lens((x, y, z)))

    def surface_normal(radial: float, angle: float, upper: bool) -> Vec3:
        x = radial * math.cos(angle)
        z = radial * math.sin(angle)
        slope_x = 2.0 * half_thickness * x / (radius * radius)
        slope_z = 2.0 * half_thickness * z / (radius * radius)
        y = 1.0 if upper else -1.0
        return normalize(rotate_lens((slope_x, y, slope_z)))

    top_center_v = len(mesh.positions)
    mesh.positions.append(position(0.0, 0.0, True))
    top_center_n = len(mesh.normals)
    mesh.normals.append(surface_normal(0.0, 0.0, True))
    top_vertices: list[list[int]] = []
    top_normals: list[list[int]] = []

    for step in range(1, radial_steps + 1):
        radial = radius * step / radial_steps
        vertex_ring: list[int] = []
        normal_ring: list[int] = []
        for k in range(segments):
            angle = 2.0 * math.pi * k / segments
            vertex_ring.append(len(mesh.positions))
            mesh.positions.append(position(radial, angle, True))
            normal_ring.append(len(mesh.normals))
            mesh.normals.append(surface_normal(radial, angle, True))
        top_vertices.append(vertex_ring)
        top_normals.append(normal_ring)

    bottom_center_v = len(mesh.positions)
    mesh.positions.append(position(0.0, 0.0, False))
    bottom_center_n = len(mesh.normals)
    mesh.normals.append(surface_normal(0.0, 0.0, False))
    bottom_vertices: list[list[int]] = []
    bottom_normals: list[list[int]] = []

    for step in range(1, radial_steps + 1):
        radial = radius * step / radial_steps
        vertex_ring = []
        normal_ring = []
        for k in range(segments):
            angle = 2.0 * math.pi * k / segments
            if step == radial_steps:
                vertex_ring.append(top_vertices[-1][k])
            else:
                vertex_ring.append(len(mesh.positions))
                mesh.positions.append(position(radial, angle, False))
            normal_ring.append(len(mesh.normals))
            mesh.normals.append(surface_normal(radial, angle, False))
        bottom_vertices.append(vertex_ring)
        bottom_normals.append(normal_ring)

    for upper, center_v, center_n, rings_v, rings_n in (
        (True, top_center_v, top_center_n, top_vertices, top_normals),
        (False, bottom_center_v, bottom_center_n,
         bottom_vertices, bottom_normals),
    ):
        first_v = rings_v[0]
        first_n = rings_n[0]
        for k in range(segments):
            nxt = (k + 1) % segments
            mesh.smooth_face([
                (center_v, center_n),
                (first_v[k], first_n[k]),
                (first_v[nxt], first_n[nxt]),
            ])
        for ring in range(radial_steps - 1):
            inner_v, outer_v = rings_v[ring], rings_v[ring + 1]
            inner_n, outer_n = rings_n[ring], rings_n[ring + 1]
            for k in range(segments):
                nxt = (k + 1) % segments
                mesh.smooth_face([
                    (inner_v[k], inner_n[k]),
                    (outer_v[k], outer_n[k]),
                    (outer_v[nxt], outer_n[nxt]),
                ])
                mesh.smooth_face([
                    (inner_v[k], inner_n[k]),
                    (outer_v[nxt], outer_n[nxt]),
                    (inner_v[nxt], inner_n[nxt]),
                ])
    return mesh


def make_gem() -> Mesh:
    mesh = Mesh("glass_brilliant_gem", "glass_gem")
    segments = 16
    center = (1.14, 0.61, -0.20)
    rings_spec = (
        (0.16, 0.41, 0.0),
        (0.25, 0.30, 0.5),
        (0.37, 0.09, 0.0),
        (0.37, 0.05, 0.0),
        (0.22, -0.23, 0.5),
    )
    rings: list[list[int]] = []
    for radius, y, phase in rings_spec:
        ring: list[int] = []
        for k in range(segments):
            angle = 2.0 * math.pi * (k + phase) / segments
            ring.append(len(mesh.positions))
            mesh.positions.append(add(center, (
                radius * math.cos(angle), y, radius * math.sin(angle),
            )))
        rings.append(ring)

    table_center = len(mesh.positions)
    mesh.positions.append(add(center, (0.0, rings_spec[0][1], 0.0)))
    culet = len(mesh.positions)
    mesh.positions.append(add(center, (0.0, -0.41, 0.0)))

    for k in range(segments):
        nxt = (k + 1) % segments
        mesh.flat_face([table_center, rings[0][k], rings[0][nxt]], center)
    for ring in range(len(rings) - 1):
        upper = rings[ring]
        lower = rings[ring + 1]
        for k in range(segments):
            nxt = (k + 1) % segments
            mesh.flat_face([upper[k], lower[k], lower[nxt]], center)
            mesh.flat_face([upper[k], lower[nxt], upper[nxt]], center)
    for k in range(segments):
        nxt = (k + 1) % segments
        mesh.flat_face([rings[-1][k], culet, rings[-1][nxt]], center)
    return mesh


def make_supports() -> list[Mesh]:
    metal = "metal_support"

    knot = Mesh("stand_knot", metal)
    for box in (
        make_box("part", metal, (-0.46, 0.012, -0.31),
                 (0.46, 0.070, -0.09)),
        make_box("part", metal, (-0.33, 0.070, -0.225),
                 (-0.285, 0.50, -0.175)),
        make_box("part", metal, (0.285, 0.070, -0.225),
                 (0.33, 0.50, -0.175)),
    ):
        merge(knot, box)

    lens = Mesh("stand_lens", metal)
    for box in (
        make_box("part", metal, (-1.65, 0.012, -0.31),
                 (-0.68, 0.070, -0.09)),
        make_box("part", metal, (-1.64, 0.070, -0.235),
                 (-1.59, 0.46, -0.165)),
        make_box("part", metal, (-0.74, 0.070, -0.235),
                 (-0.69, 0.46, -0.165)),
        make_box("part", metal, (-1.61, 0.37, -0.235),
                 (-0.72, 0.42, -0.165)),
    ):
        merge(lens, box)

    gem = make_box(
        "stand_gem", metal,
        (0.98, 0.012, -0.36), (1.30, 0.16, -0.04),
    )
    return [knot, lens, gem]


def make_light_and_plates() -> list[Mesh]:
    light = make_quad(
        "area_panel", "area_warm",
        [(-1.55, 2.60, -0.85), (1.55, 2.60, -0.85),
         (1.55, 2.60, 0.25), (-1.55, 2.60, 0.25)],
        (0.0, -1.0, 0.0),
    )

    frame = Mesh("area_panel_frame", "metal_support")
    for box in (
        make_box("part", "metal_support", (-1.63, 2.575, -0.93),
                 (-1.55, 2.64, 0.33)),
        make_box("part", "metal_support", (1.55, 2.575, -0.93),
                 (1.63, 2.64, 0.33)),
        make_box("part", "metal_support", (-1.55, 2.575, -0.93),
                 (1.55, 2.64, -0.85)),
        make_box("part", "metal_support", (-1.55, 2.575, 0.25),
                 (1.55, 2.64, 0.33)),
    ):
        merge(frame, box)

    plates = [
        make_quad(
            "caustic_plate_lens", "caustic_plate",
            [(-1.61, 0.006, -0.62), (-0.77, 0.006, -0.62),
             (-0.77, 0.006, 0.22), (-1.61, 0.006, 0.22)],
            (0.0, 1.0, 0.0),
        ),
        make_quad(
            "caustic_plate_knot", "caustic_plate",
            [(-0.58, 0.006, -0.66), (0.58, 0.006, -0.66),
             (0.58, 0.006, 0.26), (-0.58, 0.006, 0.26)],
            (0.0, 1.0, 0.0),
        ),
        make_quad(
            "caustic_plate_gem", "caustic_plate",
            [(0.72, 0.006, -0.62), (1.56, 0.006, -0.62),
             (1.56, 0.006, 0.22), (0.72, 0.006, 0.22)],
            (0.0, 1.0, 0.0),
        ),
    ]
    return [light, frame, *plates]


MATERIALS: dict[str, dict[str, tuple[float, ...] | float]] = {
    "ivory_floor": {
        "Ka": (0.018, 0.016, 0.013), "Kd": (0.38, 0.35, 0.29),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "ivory_ceiling": {
        "Ka": (0.018, 0.018, 0.016), "Kd": (0.35, 0.34, 0.31),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "ivory_back": {
        "Ka": (0.022, 0.020, 0.018), "Kd": (0.45, 0.42, 0.37),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "burgundy_wall": {
        "Ka": (0.022, 0.004, 0.007), "Kd": (0.30, 0.035, 0.055),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "blue_wall": {
        "Ka": (0.004, 0.009, 0.022), "Kd": (0.03, 0.12, 0.32),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "caustic_plate": {
        "Ka": (0.03, 0.028, 0.024), "Kd": (0.72, 0.68, 0.58),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "metal_support": {
        "Ka": (0.015, 0.014, 0.012), "Kd": (0.0, 0.0, 0.0),
        "Ks": (0.78, 0.70, 0.54), "Ns": 100.0,
        "Ke": (0.0, 0.0, 0.0),
    },
    "glass_knot": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (1.0, 1.0, 1.0), "Ns": 500.0, "Ke": (0.0, 0.0, 0.0),
    },
    "glass_lens": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (1.0, 1.0, 1.0), "Ns": 500.0, "Ke": (0.0, 0.0, 0.0),
    },
    "glass_gem": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (1.0, 1.0, 1.0), "Ns": 500.0, "Ke": (0.0, 0.0, 0.0),
    },
    "area_warm": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (0.0, 0.0, 0.0), "Ns": 0.0, "Ke": (20.0, 18.0, 15.0),
    },
}


def number(value: float) -> str:
    if abs(value - round(value)) < 1.0e-10:
        return str(int(round(value)))
    return f"{value:.6f}".rstrip("0").rstrip(".")


def write_mtl(path: Path) -> None:
    lines = ["# Crystal Optics Gallery materials"]
    for name, values in MATERIALS.items():
        lines.extend(("", f"newmtl {name}"))
        for key in ("Ka", "Kd", "Ks"):
            lines.append(f"{key} " + " ".join(number(v) for v in values[key]))
        lines.append(f"Ns {number(float(values['Ns']))}")
        lines.append("Ke " + " ".join(number(v) for v in values["Ke"]))
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def write_obj(path: Path, meshes: list[Mesh]) -> None:
    lines = [
        "# Crystal Optics Gallery",
        "# Deterministically generated; all faces are triangles with normals.",
        "mtllib crystal_optics_gallery.mtl",
    ]
    vertex_offset = 0
    normal_offset = 0
    for mesh in meshes:
        lines.extend(("", f"o {mesh.name}", f"usemtl {mesh.material}"))
        for x, y, z in mesh.positions:
            lines.append(f"v {x:.6f} {y:.6f} {z:.6f}")
        for x, y, z in mesh.normals:
            lines.append(f"vn {x:.6f} {y:.6f} {z:.6f}")
        for face in mesh.faces:
            corners = [
                f"{vertex_offset + vertex + 1}//{normal_offset + normal + 1}"
                for vertex, normal in face
            ]
            lines.append("f " + " ".join(corners))
        vertex_offset += len(mesh.positions)
        normal_offset += len(mesh.normals)
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def face_area(mesh: Mesh, face: Face) -> float:
    p0, p1, p2 = (mesh.positions[corner[0]] for corner in face)
    return 0.5 * length(cross(sub(p1, p0), sub(p2, p0)))


def validate_closed_glass(mesh: Mesh) -> None:
    edge_uses: dict[tuple[int, int], list[tuple[int, int]]] = {}
    volume = 0.0
    for face in mesh.faces:
        vertices = [corner[0] for corner in face]
        p0, p1, p2 = (mesh.positions[v] for v in vertices)
        volume += dot(p0, cross(p1, p2)) / 6.0
        geometric = cross(sub(p1, p0), sub(p2, p0))
        expected = mean(mesh.normals[n] for _, n in face)
        assert dot(geometric, expected) > 1.0e-10, (
            f"{mesh.name} has a face opposed to its supplied normals"
        )
        for a, b in zip(vertices, vertices[1:] + vertices[:1]):
            edge_uses.setdefault(tuple(sorted((a, b))), []).append((a, b))
    assert volume > 1.0e-7, f"{mesh.name} does not have positive signed volume"
    for edge, uses in edge_uses.items():
        assert len(uses) == 2, f"{mesh.name} edge {edge} has {len(uses)} uses"
        assert uses[0] == tuple(reversed(uses[1])), (
            f"{mesh.name} edge {edge} is not oppositely wound"
        )


def bounds(mesh: Mesh) -> tuple[Vec3, Vec3]:
    return (
        tuple(min(p[axis] for p in mesh.positions) for axis in range(3)),
        tuple(max(p[axis] for p in mesh.positions) for axis in range(3)),
    )


def validate_nonoverlap(glass: list[Mesh]) -> None:
    for first_index, first in enumerate(glass):
        low_a, high_a = bounds(first)
        for second in glass[first_index + 1:]:
            low_b, high_b = bounds(second)
            separated = any(
                high_a[axis] < low_b[axis] - 1.0e-5
                or high_b[axis] < low_a[axis] - 1.0e-5
                for axis in range(3)
            )
            assert separated, f"glass bounds overlap: {first.name}, {second.name}"


def validate_material_file(path: Path) -> None:
    blocks: dict[str, list[str]] = {}
    current = ""
    for line in path.read_text(encoding="ascii").splitlines():
        if line.startswith("newmtl "):
            current = line.split(maxsplit=1)[1]
            blocks[current] = []
        elif line and not line.startswith("#"):
            assert current
            blocks[current].append(line)
    assert set(blocks) == set(MATERIALS)
    for name, lines in blocks.items():
        keys = [line.split()[0] for line in lines]
        assert keys == ["Ka", "Kd", "Ks", "Ns", "Ke"], name
        assert keys.count("Ke") == 1
    for name in ("glass_knot", "glass_lens", "glass_gem"):
        assert name.startswith("glass")
        ks = next(line for line in blocks[name] if line.startswith("Ks "))
        assert ks == "Ks 1 1 1"
    ns = next(line for line in blocks["metal_support"] if line.startswith("Ns "))
    assert ns == "Ns 100"
    ke = next(line for line in blocks["area_warm"] if line.startswith("Ke "))
    assert ke == "Ke 20 18 15"


def validate_obj_file(path: Path, meshes: list[Mesh]) -> None:
    lines = path.read_text(encoding="ascii").splitlines()
    assert max(map(len, lines)) <= 78
    vertices = sum(line.startswith("v ") for line in lines)
    normals = sum(line.startswith("vn ") for line in lines)
    faces = [line for line in lines if line.startswith("f ")]
    objects = [line[2:] for line in lines if line.startswith("o ")]
    materials = [line[7:] for line in lines if line.startswith("usemtl ")]
    assert objects == [mesh.name for mesh in meshes]
    assert set(materials).issubset(MATERIALS)
    assert len(faces) < 5000
    pattern = re.compile(r"^[1-9][0-9]*//[1-9][0-9]*$")
    for line in faces:
        fields = line.split()
        assert len(fields) == 4
        for corner in fields[1:]:
            assert pattern.fullmatch(corner), corner
            vertex, normal = (int(value) for value in corner.split("//"))
            assert 1 <= vertex <= vertices
            assert 1 <= normal <= normals
    for mesh in meshes:
        assert mesh.positions and mesh.normals and mesh.faces
        for face in mesh.faces:
            assert face_area(mesh, face) > 1.0e-10, mesh.name


def validate_scene(meshes: list[Mesh], obj_path: Path, mtl_path: Path) -> None:
    glass = [mesh for mesh in meshes if mesh.material.startswith("glass")]
    assert len(glass) == 3
    for mesh in glass:
        validate_closed_glass(mesh)
    validate_nonoverlap(glass)

    area = next(mesh for mesh in meshes if mesh.name == "area_panel")
    for face in area.faces:
        p0, p1, p2 = (area.positions[v] for v, _ in face)
        normal = normalize(cross(sub(p1, p0), sub(p2, p0)))
        assert normal[1] < -0.999999

    expected_gallery = {
        "gallery_floor": (0.0, 1.0, 0.0),
        "gallery_ceiling": (0.0, -1.0, 0.0),
        "gallery_back": (0.0, 0.0, 1.0),
        "gallery_left": (1.0, 0.0, 0.0),
        "gallery_right": (-1.0, 0.0, 0.0),
    }
    assert len(expected_gallery) == 5
    for mesh in meshes:
        if mesh.name in expected_gallery:
            desired = expected_gallery[mesh.name]
            for face in mesh.faces:
                p0, p1, p2 = (mesh.positions[v] for v, _ in face)
                normal = normalize(cross(sub(p1, p0), sub(p2, p0)))
                assert dot(normal, desired) > 0.999999

    validate_obj_file(obj_path, meshes)
    validate_material_file(mtl_path)


def main() -> None:
    output_dir = Path(__file__).resolve().parent
    obj_path = output_dir / "crystal_optics_gallery.obj"
    mtl_path = output_dir / "crystal_optics_gallery.mtl"

    meshes = [
        *make_gallery(),
        *make_light_and_plates(),
        *make_supports(),
        make_trefoil(),
        make_lens(),
        make_gem(),
    ]
    write_mtl(mtl_path)
    write_obj(obj_path, meshes)
    validate_scene(meshes, obj_path, mtl_path)

    total_faces = sum(len(mesh.faces) for mesh in meshes)
    total_vertices = sum(len(mesh.positions) for mesh in meshes)
    print(f"wrote {obj_path.name} and {mtl_path.name}")
    print(f"objects={len(meshes)} materials={len(MATERIALS)}")
    print(f"vertices={total_vertices} triangles={total_faces}")
    for mesh in meshes:
        print(f"{mesh.name}: {len(mesh.faces)} triangles ({mesh.material})")


if __name__ == "__main__":
    main()
