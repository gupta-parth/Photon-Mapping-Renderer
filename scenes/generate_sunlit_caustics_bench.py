#!/usr/bin/env python3
"""Generate and validate the standalone Sunlit Caustics Bench scene."""

from __future__ import annotations

import math
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


Vec3 = tuple[float, float, float]
Corner = tuple[int, int]
Face = tuple[Corner, Corner, Corner]

ROOM_LOW: Vec3 = (-3.20, 0.0, -1.80)
ROOM_HIGH: Vec3 = (3.20, 2.65, 1.90)
LIGHT_CENTER: Vec3 = (0.0, 2.25, -1.75)
LIGHT_TARGET: Vec3 = (0.0, 0.72, -0.50)
LIGHT_SIZE = 0.08
LENS_CENTER: Vec3 = (-1.10, 0.676, -0.50)
KNOT_CENTER: Vec3 = (0.0, 0.72, -0.50)
GEM_CENTER: Vec3 = (1.10, 0.48, -0.50)
CAMERA_EYE: Vec3 = (0.0, 1.65, 3.50)
CAMERA_LOOK_AT: Vec3 = (0.0, 0.38, -0.20)
CAMERA_UP: Vec3 = (0.0, 1.0, 0.0)
CAMERA_FOV_Y = 45.0
CAMERA_ASPECT = 4.0 / 3.0


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
    magnitude = length(a)
    if magnitude <= 1.0e-12:
        raise ValueError("cannot normalize a zero-length vector")
    return mul(a, 1.0 / magnitude)


def mean(points: Iterable[Vec3]) -> Vec3:
    values = list(points)
    count = float(len(values))
    return (
        sum(point[0] for point in values) / count,
        sum(point[1] for point in values) / count,
        sum(point[2] for point in values) / count,
    )


def rotate_y(vector: Vec3, degrees: float) -> Vec3:
    angle = math.radians(degrees)
    cosine = math.cos(angle)
    sine = math.sin(angle)
    x, y, z = vector
    return (cosine * x + sine * z, y, -sine * x + cosine * z)


@dataclass
class Mesh:
    name: str
    material: str
    positions: list[Vec3] = field(default_factory=list)
    normals: list[Vec3] = field(default_factory=list)
    faces: list[Face] = field(default_factory=list)

    def smooth_face(self, corners: list[Corner]) -> None:
        p0, p1, p2 = (self.positions[vertex] for vertex, _ in corners)
        expected = mean(self.normals[normal] for _, normal in corners)
        geometric = cross(sub(p1, p0), sub(p2, p0))
        if dot(geometric, expected) < 0.0:
            corners[1], corners[2] = corners[2], corners[1]
        self.faces.append((corners[0], corners[1], corners[2]))

    def flat_face(self, vertices: list[int], interior: Vec3) -> None:
        p0, p1, p2 = (self.positions[vertex] for vertex in vertices)
        geometric = cross(sub(p1, p0), sub(p2, p0))
        centroid = mean((p0, p1, p2))
        if dot(geometric, sub(centroid, interior)) < 0.0:
            vertices[1], vertices[2] = vertices[2], vertices[1]
            p1, p2 = p2, p1
            geometric = cross(sub(p1, p0), sub(p2, p0))
        normal_index = len(self.normals)
        self.normals.append(normalize(geometric))
        self.faces.append(tuple((vertex, normal_index) for vertex in vertices))


def merge(target: Mesh, source: Mesh) -> None:
    vertex_offset = len(target.positions)
    normal_offset = len(target.normals)
    target.positions.extend(source.positions)
    target.normals.extend(source.normals)
    for face in source.faces:
        target.faces.append(tuple(
            (vertex_offset + vertex, normal_offset + normal)
            for vertex, normal in face
        ))


def make_quad(
    name: str,
    material: str,
    points: list[Vec3],
    desired_normal: Vec3,
) -> Mesh:
    mesh = Mesh(name, material, positions=points)
    normal = normalize(desired_normal)
    mesh.normals.append(normal)
    for vertices in ([0, 1, 2], [0, 2, 3]):
        p0, p1, p2 = (mesh.positions[vertex] for vertex in vertices)
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
    for a, b, c, d in (
        (0, 3, 2, 1), (4, 5, 6, 7),
        (0, 4, 7, 3), (1, 2, 6, 5),
        (0, 1, 5, 4), (3, 7, 6, 2),
    ):
        mesh.flat_face([a, b, c], center)
        mesh.flat_face([a, c, d], center)
    return mesh


def make_gallery() -> list[Mesh]:
    x0, y0, z0 = ROOM_LOW
    x1, y1, z1 = ROOM_HIGH
    return [
        make_quad(
            "gallery_floor", "warm_gray_floor",
            [(x0, y0, z0), (x1, y0, z0),
             (x1, y0, z1), (x0, y0, z1)],
            (0.0, 1.0, 0.0),
        ),
        make_quad(
            "gallery_ceiling", "neutral_ceiling",
            [(x0, y1, z1), (x1, y1, z1),
             (x1, y1, z0), (x0, y1, z0)],
            (0.0, -1.0, 0.0),
        ),
        make_quad(
            "gallery_back", "neutral_back",
            [(x0, y0, z0), (x0, y1, z0),
             (x1, y1, z0), (x1, y0, z0)],
            (0.0, 0.0, 1.0),
        ),
        make_quad(
            "gallery_left", "muted_umber_wall",
            [(x0, y0, z1), (x0, y1, z1),
             (x0, y1, z0), (x0, y0, z0)],
            (1.0, 0.0, 0.0),
        ),
        make_quad(
            "gallery_right", "muted_slate_wall",
            [(x1, y0, z0), (x1, y1, z0),
             (x1, y1, z1), (x1, y0, z1)],
            (-1.0, 0.0, 0.0),
        ),
    ]


def make_area_light() -> Mesh:
    direction = normalize(sub(LIGHT_TARGET, LIGHT_CENTER))
    basis_u = (1.0, 0.0, 0.0)
    basis_v = normalize(cross(direction, basis_u))
    half_u = mul(basis_u, 0.5 * LIGHT_SIZE)
    half_v = mul(basis_v, 0.5 * LIGHT_SIZE)
    points = [
        sub(sub(LIGHT_CENTER, half_u), half_v),
        add(sub(LIGHT_CENTER, half_v), half_u),
        add(add(LIGHT_CENTER, half_u), half_v),
        add(sub(LIGHT_CENTER, half_u), half_v),
    ]
    return make_quad("area_key", "area_sun", points, direction)


def lens_basis() -> tuple[Vec3, Vec3, Vec3]:
    # Local +Y faces the source; transmitted light travels toward local -Y.
    axis = normalize(sub(LIGHT_CENTER, LENS_CENTER))
    basis_u = normalize(cross((0.0, 1.0, 0.0), axis))
    basis_v = normalize(cross(basis_u, axis))
    return basis_u, axis, basis_v


def make_lens() -> Mesh:
    mesh = Mesh("glass_focused_lens", "glass_lens")
    segments = 48
    radial_steps = 6
    radius = 0.34
    half_thickness = 0.0587
    basis_u, axis, basis_v = lens_basis()

    def transform(local: Vec3) -> Vec3:
        return add(LENS_CENTER, add(
            add(mul(basis_u, local[0]), mul(axis, local[1])),
            mul(basis_v, local[2]),
        ))

    def position(radial: float, angle: float, source_side: bool) -> Vec3:
        x = radial * math.cos(angle)
        z = radial * math.sin(angle)
        sign = 1.0 if source_side else -1.0
        y = sign * half_thickness * (1.0 - (radial / radius) ** 2)
        return transform((x, y, z))

    def surface_normal(
        radial: float,
        angle: float,
        source_side: bool,
    ) -> Vec3:
        x = radial * math.cos(angle)
        z = radial * math.sin(angle)
        slope_x = 2.0 * half_thickness * x / (radius * radius)
        slope_z = 2.0 * half_thickness * z / (radius * radius)
        y = 1.0 if source_side else -1.0
        return normalize(add(
            add(mul(basis_u, slope_x), mul(axis, y)),
            mul(basis_v, slope_z),
        ))

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
        for segment in range(segments):
            angle = 2.0 * math.pi * segment / segments
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
        for segment in range(segments):
            angle = 2.0 * math.pi * segment / segments
            if step == radial_steps:
                vertex_ring.append(top_vertices[-1][segment])
            else:
                vertex_ring.append(len(mesh.positions))
                mesh.positions.append(position(radial, angle, False))
            normal_ring.append(len(mesh.normals))
            mesh.normals.append(surface_normal(radial, angle, False))
        bottom_vertices.append(vertex_ring)
        bottom_normals.append(normal_ring)

    for center_v, center_n, rings_v, rings_n in (
        (top_center_v, top_center_n, top_vertices, top_normals),
        (bottom_center_v, bottom_center_n, bottom_vertices, bottom_normals),
    ):
        first_v = rings_v[0]
        first_n = rings_n[0]
        for segment in range(segments):
            nxt = (segment + 1) % segments
            mesh.smooth_face([
                (center_v, center_n),
                (first_v[segment], first_n[segment]),
                (first_v[nxt], first_n[nxt]),
            ])
        for ring in range(radial_steps - 1):
            inner_v, outer_v = rings_v[ring], rings_v[ring + 1]
            inner_n, outer_n = rings_n[ring], rings_n[ring + 1]
            for segment in range(segments):
                nxt = (segment + 1) % segments
                mesh.smooth_face([
                    (inner_v[segment], inner_n[segment]),
                    (outer_v[segment], outer_n[segment]),
                    (outer_v[nxt], outer_n[nxt]),
                ])
                mesh.smooth_face([
                    (inner_v[segment], inner_n[segment]),
                    (outer_v[nxt], outer_n[nxt]),
                    (inner_v[nxt], inner_n[nxt]),
                ])
    return mesh


def lens_post_contact() -> Vec3:
    _, axis, _ = lens_basis()
    # Project a rearward/downward preference into the lens plane.  This finds a
    # low rim point hidden behind the optic without hard-coding its position.
    preference = normalize((0.0, -1.0, -0.5))
    rim_direction = normalize(sub(preference, mul(axis, dot(preference, axis))))
    return add(LENS_CENTER, mul(rim_direction, 0.34))


def make_lens_rim_post() -> Mesh:
    rim_point = lens_post_contact()
    half_width = 0.014
    return make_box(
        "lens_rear_rim_post",
        "dark_metal",
        (rim_point[0] - half_width, 0.004, rim_point[2] - half_width),
        (rim_point[0] + half_width,
         rim_point[1],
         rim_point[2] + half_width),
    )


def make_trefoil() -> Mesh:
    mesh = Mesh("glass_trefoil_knot", "glass_knot")
    path_segments = 120
    ring_segments = 12
    major_radius = 0.50
    knot_radius = 0.15
    tube_radius = 0.09
    yaw = 10.0
    horizontal_scale = 0.92

    for path_index in range(path_segments):
        t = 2.0 * math.pi * path_index / path_segments
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

        for ring_index in range(ring_segments):
            angle = 2.0 * math.pi * ring_index / ring_segments
            radial = add(
                mul(frame_normal, math.cos(angle)),
                mul(frame_binormal, math.sin(angle)),
            )
            swept_point = add(point, mul(radial, tube_radius))
            swept_point = (
                horizontal_scale * swept_point[0],
                swept_point[1],
                swept_point[2],
            )
            # The inverse scale gives the correct normal for the subtle
            # horizontal compression used to preserve spacing from the gem.
            scaled_normal = normalize((
                radial[0] / horizontal_scale,
                radial[1],
                radial[2],
            ))
            mesh.positions.append(add(
                KNOT_CENTER,
                rotate_y(swept_point, yaw),
            ))
            mesh.normals.append(normalize(rotate_y(scaled_normal, yaw)))

    def index(path_index: int, ring_index: int) -> int:
        return (
            (path_index % path_segments) * ring_segments
            + ring_index % ring_segments
        )

    for path_index in range(path_segments):
        for ring_index in range(ring_segments):
            a = index(path_index, ring_index)
            b = index(path_index + 1, ring_index)
            c = index(path_index + 1, ring_index + 1)
            d = index(path_index, ring_index + 1)
            mesh.smooth_face([(a, a), (b, b), (c, c)])
            mesh.smooth_face([(a, a), (c, c), (d, d)])
    return mesh


def make_gem() -> Mesh:
    mesh = Mesh("glass_brilliant_gem", "glass_gem")
    segments = 16
    yaw = 2.5
    rings_spec = (
        (0.18, 0.47, 0.0),
        (0.29, 0.34, 0.5),
        (0.42, 0.10, 0.0),
        (0.42, 0.05, 0.0),
        (0.25, -0.26, 0.5),
    )
    rings: list[list[int]] = []
    for radius, height, phase in rings_spec:
        ring: list[int] = []
        for segment in range(segments):
            angle = 2.0 * math.pi * (segment + phase) / segments
            local = (
                radius * math.cos(angle),
                height,
                radius * math.sin(angle),
            )
            ring.append(len(mesh.positions))
            mesh.positions.append(add(GEM_CENTER, rotate_y(local, yaw)))
        rings.append(ring)

    table_center = len(mesh.positions)
    mesh.positions.append(add(GEM_CENTER, (0.0, rings_spec[0][1], 0.0)))
    culet = len(mesh.positions)
    mesh.positions.append((GEM_CENTER[0], 0.01, GEM_CENTER[2]))

    for segment in range(segments):
        nxt = (segment + 1) % segments
        mesh.flat_face(
            [table_center, rings[0][segment], rings[0][nxt]],
            GEM_CENTER,
        )
    for ring_index in range(len(rings) - 1):
        upper = rings[ring_index]
        lower = rings[ring_index + 1]
        for segment in range(segments):
            nxt = (segment + 1) % segments
            mesh.flat_face(
                [upper[segment], lower[segment], lower[nxt]],
                GEM_CENTER,
            )
            mesh.flat_face(
                [upper[segment], lower[nxt], upper[nxt]],
                GEM_CENTER,
            )
    for segment in range(segments):
        nxt = (segment + 1) % segments
        mesh.flat_face(
            [rings[-1][segment], culet, rings[-1][nxt]],
            GEM_CENTER,
        )
    return mesh


MATERIALS: dict[str, dict[str, tuple[float, ...] | float]] = {
    "warm_gray_floor": {
        "Ka": (0.018, 0.017, 0.015), "Kd": (0.52, 0.50, 0.46),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "neutral_ceiling": {
        "Ka": (0.012, 0.012, 0.011), "Kd": (0.30, 0.29, 0.27),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "neutral_back": {
        "Ka": (0.016, 0.015, 0.014), "Kd": (0.38, 0.36, 0.33),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "muted_umber_wall": {
        "Ka": (0.012, 0.006, 0.005), "Kd": (0.22, 0.09, 0.07),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "muted_slate_wall": {
        "Ka": (0.005, 0.008, 0.012), "Kd": (0.07, 0.13, 0.22),
        "Ks": (0.0, 0.0, 0.0), "Ns": 10.0, "Ke": (0.0, 0.0, 0.0),
    },
    "dark_metal": {
        "Ka": (0.008, 0.008, 0.007), "Kd": (0.025, 0.024, 0.022),
        "Ks": (0.52, 0.49, 0.42), "Ns": 120.0, "Ke": (0.0, 0.0, 0.0),
    },
    "glass_lens": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (1.0, 1.0, 1.0), "Ns": 500.0, "Ke": (0.0, 0.0, 0.0),
    },
    "glass_knot": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (1.0, 1.0, 1.0), "Ns": 500.0, "Ke": (0.0, 0.0, 0.0),
    },
    "glass_gem": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (1.0, 1.0, 1.0), "Ns": 500.0, "Ke": (0.0, 0.0, 0.0),
    },
    "area_sun": {
        "Ka": (0.0, 0.0, 0.0), "Kd": (0.0, 0.0, 0.0),
        "Ks": (0.0, 0.0, 0.0), "Ns": 0.0,
        "Ke": (4700.0, 4230.0, 3525.0),
    },
}


def number(value: float) -> str:
    if abs(value - round(value)) < 1.0e-10:
        return str(int(round(value)))
    return f"{value:.6f}".rstrip("0").rstrip(".")


def write_mtl(path: Path) -> None:
    lines = ["# Sunlit Caustics Bench materials"]
    for name, values in MATERIALS.items():
        lines.extend(("", f"newmtl {name}"))
        for key in ("Ka", "Kd", "Ks"):
            lines.append(
                f"{key} " + " ".join(number(value) for value in values[key])
            )
        lines.append(f"Ns {number(float(values['Ns']))}")
        lines.append(
            "Ke " + " ".join(number(value) for value in values["Ke"])
        )
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def write_obj(path: Path, meshes: list[Mesh]) -> None:
    lines = [
        "# Sunlit Caustics Bench",
        "# Deterministically generated; triangle faces and complete normals.",
        "mtllib sunlit_caustics_bench.mtl",
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
    p0, p1, p2 = (mesh.positions[vertex] for vertex, _ in face)
    return 0.5 * length(cross(sub(p1, p0), sub(p2, p0)))


def bounds(mesh: Mesh) -> tuple[Vec3, Vec3]:
    return (
        tuple(min(point[axis] for point in mesh.positions) for axis in range(3)),
        tuple(max(point[axis] for point in mesh.positions) for axis in range(3)),
    )


def validate_closed_glass(mesh: Mesh) -> None:
    edge_uses: dict[tuple[int, int], list[tuple[int, int]]] = {}
    signed_volume = 0.0
    for face in mesh.faces:
        vertices = [corner[0] for corner in face]
        p0, p1, p2 = (mesh.positions[vertex] for vertex in vertices)
        signed_volume += dot(p0, cross(p1, p2)) / 6.0
        geometric = cross(sub(p1, p0), sub(p2, p0))
        expected = mean(mesh.normals[normal] for _, normal in face)
        assert dot(geometric, expected) > 1.0e-10, (
            f"{mesh.name} face opposes its supplied normal"
        )
        for a, b in zip(vertices, vertices[1:] + vertices[:1]):
            edge_uses.setdefault(tuple(sorted((a, b))), []).append((a, b))
    assert signed_volume > 1.0e-7, f"{mesh.name} has non-positive volume"
    for edge, uses in edge_uses.items():
        assert len(uses) == 2, f"{mesh.name} edge {edge} has {len(uses)} uses"
        assert uses[0] == tuple(reversed(uses[1])), (
            f"{mesh.name} edge {edge} is not oppositely wound"
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


def axial_floor_target(source: Vec3, center: Vec3) -> Vec3:
    travel = sub(center, source)
    scale = -center[1] / travel[1]
    return add(center, mul(travel, scale))


def camera_corner_direction(horizontal: float, vertical: float) -> Vec3:
    forward = normalize(sub(CAMERA_LOOK_AT, CAMERA_EYE))
    right = normalize(cross(forward, CAMERA_UP))
    up = normalize(cross(right, forward))
    half_height = math.tan(0.5 * math.radians(CAMERA_FOV_Y))
    half_width = CAMERA_ASPECT * half_height
    return normalize(add(
        forward,
        add(mul(right, horizontal * half_width), mul(up, vertical * half_height)),
    ))


def project_camera(point: Vec3) -> tuple[float, float]:
    """Return camera-space normalized device coordinates for a world point."""
    forward = normalize(sub(CAMERA_LOOK_AT, CAMERA_EYE))
    right = normalize(cross(forward, CAMERA_UP))
    up = normalize(cross(right, forward))
    offset = sub(point, CAMERA_EYE)
    depth = dot(offset, forward)
    assert depth > 0.0
    half_height = math.tan(0.5 * math.radians(CAMERA_FOV_Y))
    return (
        dot(offset, right) / (depth * CAMERA_ASPECT * half_height),
        dot(offset, up) / (depth * half_height),
    )


def ray_hits_gallery(origin: Vec3, direction: Vec3) -> bool:
    planes = (
        (1, ROOM_LOW[1]), (1, ROOM_HIGH[1]),
        (2, ROOM_LOW[2]), (0, ROOM_LOW[0]), (0, ROOM_HIGH[0]),
    )
    for axis, coordinate in planes:
        if abs(direction[axis]) <= 1.0e-10:
            continue
        distance = (coordinate - origin[axis]) / direction[axis]
        if distance <= 0.0:
            continue
        point = add(origin, mul(direction, distance))
        if all(
            ROOM_LOW[index] - 1.0e-7 <= point[index]
            <= ROOM_HIGH[index] + 1.0e-7
            for index in range(3)
        ):
            return True
    return False


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
        assert [line.split()[0] for line in lines] == [
            "Ka", "Kd", "Ks", "Ns", "Ke",
        ], name
    floor_kd = next(
        line for line in blocks["warm_gray_floor"] if line.startswith("Kd ")
    )
    assert floor_kd == "Kd 0.52 0.5 0.46"
    emitter_ke = next(
        line for line in blocks["area_sun"] if line.startswith("Ke ")
    )
    assert emitter_ke == "Ke 4700 4230 3525"
    for name in ("glass_lens", "glass_knot", "glass_gem"):
        ks = next(line for line in blocks[name] if line.startswith("Ks "))
        assert ks == "Ks 1 1 1"


def validate_obj_file(path: Path, meshes: list[Mesh]) -> None:
    lines = path.read_text(encoding="ascii").splitlines()
    assert lines[2] == "mtllib sunlit_caustics_bench.mtl"
    assert max(map(len, lines)) <= 78
    vertex_count = sum(line.startswith("v ") for line in lines)
    normal_count = sum(line.startswith("vn ") for line in lines)
    faces = [line for line in lines if line.startswith("f ")]
    assert [line[2:] for line in lines if line.startswith("o ")] == [
        mesh.name for mesh in meshes
    ]
    assert len(faces) < 5000
    pattern = re.compile(r"^[1-9][0-9]*//[1-9][0-9]*$")
    for line in faces:
        fields = line.split()
        assert len(fields) == 4
        for corner in fields[1:]:
            assert pattern.fullmatch(corner), corner
            vertex, normal = (int(value) for value in corner.split("//"))
            assert 1 <= vertex <= vertex_count
            assert 1 <= normal <= normal_count
    for mesh in meshes:
        assert mesh.positions and mesh.normals and mesh.faces
        assert mesh.material in MATERIALS
        for face in mesh.faces:
            assert face_area(mesh, face) > 1.0e-10, mesh.name


def validate_scene(meshes: list[Mesh], obj_path: Path, mtl_path: Path) -> None:
    glass = [mesh for mesh in meshes if mesh.material.startswith("glass")]
    assert len(glass) == 3
    for mesh in glass:
        validate_closed_glass(mesh)
    validate_nonoverlap(glass)

    gallery = {mesh.name: mesh for mesh in meshes if mesh.name.startswith("gallery_")}
    assert set(gallery) == {
        "gallery_floor", "gallery_ceiling", "gallery_back",
        "gallery_left", "gallery_right",
    }
    scene_points = [point for mesh in gallery.values() for point in mesh.positions]
    assert bounds(Mesh("room", "", positions=scene_points)) == (
        ROOM_LOW, ROOM_HIGH,
    )
    expected_normals = {
        "gallery_floor": (0.0, 1.0, 0.0),
        "gallery_ceiling": (0.0, -1.0, 0.0),
        "gallery_back": (0.0, 0.0, 1.0),
        "gallery_left": (1.0, 0.0, 0.0),
        "gallery_right": (-1.0, 0.0, 0.0),
    }
    for name, desired in expected_normals.items():
        for face in gallery[name].faces:
            p0, p1, p2 = (
                gallery[name].positions[vertex] for vertex, _ in face
            )
            assert dot(normalize(cross(sub(p1, p0), sub(p2, p0))), desired) > 0.999999

    emitters = [
        mesh for mesh in meshes
        if MATERIALS[mesh.material]["Ke"] != (0.0, 0.0, 0.0)
    ]
    assert len(emitters) == 1
    area = emitters[0]
    assert area.name == "area_key" and len(area.positions) == 4
    assert length(sub(mean(area.positions), LIGHT_CENTER)) < 1.0e-9
    assert abs(length(sub(area.positions[1], area.positions[0])) - LIGHT_SIZE) < 1.0e-9
    assert abs(length(sub(area.positions[3], area.positions[0])) - LIGHT_SIZE) < 1.0e-9
    light_direction = normalize(sub(LIGHT_TARGET, LIGHT_CENTER))
    assert dot(area.normals[0], light_direction) > 0.999999

    lens_target = axial_floor_target(LIGHT_CENTER, LENS_CENTER)
    assert length(sub(lens_target, (-1.57, 0.0, 0.04))) < 0.01
    knot_target = axial_floor_target(LIGHT_CENTER, KNOT_CENTER)
    gem_target = axial_floor_target(LIGHT_CENTER, GEM_CENTER)
    targets = [lens_target, knot_target, gem_target]
    assert all(abs(target[1]) < 1.0e-10 for target in targets)
    assert min(
        length(sub(first, second))
        for index, first in enumerate(targets)
        for second in targets[index + 1:]
    ) > 1.25

    gem = next(mesh for mesh in glass if mesh.name == "glass_brilliant_gem")
    gem_low, gem_high = bounds(gem)
    assert abs(gem_low[1] - 0.01) < 1.0e-9
    assert abs(gem_high[1] - 0.95) < 1.0e-9
    assert max(abs(gem_low[0] - GEM_CENTER[0]),
               abs(gem_high[0] - GEM_CENTER[0])) <= 0.421
    assert max(abs(gem_low[2] - GEM_CENTER[2]),
               abs(gem_high[2] - GEM_CENTER[2])) <= 0.421

    knot = next(mesh for mesh in glass if mesh.name == "glass_trefoil_knot")
    knot_low, _ = bounds(knot)
    assert 0.009 <= knot_low[1] <= 0.03

    post = next(mesh for mesh in meshes if mesh.name == "lens_rear_rim_post")
    post_low, post_high = bounds(post)
    contact = lens_post_contact()
    assert abs(post_high[1] - contact[1]) < 1.0e-9
    assert post_low[0] < contact[0] < post_high[0]
    assert post_low[2] < contact[2] < post_high[2]
    # The support remains far behind the focused floor footprint.
    assert length((post_high[0] - lens_target[0], 0.0,
                   post_high[2] - lens_target[2])) > 0.75

    for horizontal in (-1.0, 1.0):
        for vertical in (-1.0, 1.0):
            direction = camera_corner_direction(horizontal, vertical)
            assert ray_hits_gallery(CAMERA_EYE, direction), (
                f"camera corner ({horizontal}, {vertical}) misses gallery"
            )
    seam_ndc = project_camera((0.0, 0.0, ROOM_LOW[2]))
    seam_row = 0.5 * (1.0 - seam_ndc[1]) * 1536.0
    assert abs(seam_ndc[0]) < 1.0e-12
    assert abs(seam_row - 714.5) < 1.0
    for target in targets:
        target_ndc = project_camera(target)
        target_pixel = (
            0.5 * (target_ndc[0] + 1.0) * 2048.0,
            0.5 * (1.0 - target_ndc[1]) * 1536.0,
        )
        assert 60.0 < target_pixel[0] < 2048.0 - 60.0
        assert 60.0 < target_pixel[1] < 1536.0 - 60.0
    emitter_ndc = [project_camera(point) for point in area.positions]
    assert min(point[1] for point in emitter_ndc) > 1.0

    validate_obj_file(obj_path, meshes)
    validate_material_file(mtl_path)


def main() -> None:
    output_dir = Path(__file__).resolve().parent
    obj_path = output_dir / "sunlit_caustics_bench.obj"
    mtl_path = output_dir / "sunlit_caustics_bench.mtl"
    meshes = [
        *make_gallery(),
        make_area_light(),
        make_lens_rim_post(),
        make_lens(),
        make_trefoil(),
        make_gem(),
    ]
    write_mtl(mtl_path)
    write_obj(obj_path, meshes)
    validate_scene(meshes, obj_path, mtl_path)

    total_vertices = sum(len(mesh.positions) for mesh in meshes)
    total_normals = sum(len(mesh.normals) for mesh in meshes)
    total_faces = sum(len(mesh.faces) for mesh in meshes)
    targets = [
        axial_floor_target(LIGHT_CENTER, center)
        for center in (LENS_CENTER, KNOT_CENTER, GEM_CENTER)
    ]
    print(f"wrote {obj_path.name} and {mtl_path.name}")
    print(f"objects={len(meshes)} materials={len(MATERIALS)}")
    print(
        f"vertices={total_vertices} normals={total_normals} "
        f"triangles={total_faces}"
    )
    for label, target in zip(("lens", "knot", "gem"), targets):
        print(
            f"{label}_axis_floor_target="
            f"({target[0]:.3f}, {target[1]:.3f}, {target[2]:.3f})"
        )
    for mesh in meshes:
        print(f"{mesh.name}: {len(mesh.faces)} triangles ({mesh.material})")


if __name__ == "__main__":
    main()
