#!/usr/bin/env python3
"""Curve sampling and rig evaluation for the cut camera CCB containers.

The rules are the ones in docs/resource/cut-camera-ccb.md ("Evaluation at a frame"). Arithmetic is
double precision, so a result can differ from the client's single precision values in the last
digits. The Euler order of the rotation helper is open, so the order is an argument.
"""

from __future__ import annotations

import itertools
import math
import struct

# The 8 bytes at 0x00fef0c0, 000000a046df913f: the single precision pi/180 widened to a double.
DEGREES_TO_RADIANS = struct.unpack("<d", bytes.fromhex("000000a046df913f"))[0]
TICKS_PER_FRAME = 10000.0


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


IDENTITY = [[1.0 if i == j else 0.0 for j in range(4)] for i in range(4)]


def sample_step(flag: int, keys: list[tuple[float, float]], x: float) -> float:
    """The StepCurve sample (0x00a35ce0): the held value of the last key at or below x."""
    if not keys:
        return 0.0
    last_time = keys[-1][0]
    if flag and last_time > 0.0 and x >= last_time:
        x = math.fmod(x, last_time)
    if x <= keys[0][0]:
        return keys[0][1]
    if x >= last_time:
        return keys[-1][1]
    held = keys[0][1]
    for time, value in keys:
        if time <= x:
            held = value
    return held


def sample_fixed(keys: list[tuple[float, float, float, float]], x: float) -> float:
    """The type 2 cubic Hermite sample between the bracketing pair of keys."""
    if not keys:
        return 0.0
    if x <= keys[0][0]:
        return keys[0][1]
    if x >= keys[-1][0]:
        return keys[-1][1]
    for left, right in itertools.pairwise(keys):
        if left[0] <= x < right[0]:
            if left[0] == x:
                return left[1]
            d = x - left[0]
            u = d / (right[0] - left[0])
            v = u - 1.0
            return (
                (2.0 * v - 1.0) * u * u * (left[1] - right[1])
                + left[1]
                + d * v * (left[3] * v + right[2] * u)
            )
    return keys[-1][1]


def sample_linear16(keys: list[tuple[float, float]], x: float) -> float:
    """The type 4 sample: linear between the bracketing pair of keys."""
    if not keys:
        return 0.0
    if x <= keys[0][0]:
        return keys[0][1]
    if x >= keys[-1][0]:
        return keys[-1][1]
    for left, right in itertools.pairwise(keys):
        if left[0] <= x < right[0]:
            return left[1] + (right[1] - left[1]) * (
                (x - left[0]) / (right[0] - left[0])
            )
    return keys[-1][1]


def sample(curve: tuple, x: float) -> float:
    kind = curve[0]
    if kind == "constant":
        return curve[1]
    if kind == "fixed":
        return sample_fixed(curve[1], x)
    if kind == "linear16":
        return sample_linear16(curve[1], x)
    return sample_step(curve[1], curve[2], x)


def multiply(a: list[list[float]], b: list[list[float]]) -> list[list[float]]:
    return [
        [sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)
    ]


def translation(t: tuple[float, float, float]) -> list[list[float]]:
    out = [row[:] for row in IDENTITY]
    out[3][:3] = t
    return out


def scale(s: tuple[float, float, float]) -> list[list[float]]:
    out = [row[:] for row in IDENTITY]
    for axis in range(3):
        out[axis][axis] = s[axis]
    return out


def axis_matrix(axis: int, angle: float) -> list[list[float]]:
    s, c = math.sin(angle), math.cos(angle)
    out = [row[:] for row in IDENTITY]
    if axis == 0:
        out[1][1], out[1][2], out[2][1], out[2][2] = c, s, -s, c
    elif axis == 1:
        out[0][0], out[0][2], out[2][0], out[2][2] = c, -s, s, c
    else:
        out[0][0], out[0][1], out[1][0], out[1][1] = c, s, -s, c
    return out


def rotation(
    angles: tuple[float, float, float], order: tuple[int, int, int] = (0, 1, 2)
) -> list[list[float]]:
    """The row-vector rotation of angles (radians) composed in the given axis order."""
    out = IDENTITY
    for axis in order:
        out = multiply(out, axis_matrix(axis, angles[axis]))
    return out


class Rig:
    """One parsed container: its groups by section ordinal and its camera records."""

    def __init__(self, parsed: dict) -> None:
        self.parsed = parsed
        self.groups = [
            {name: curve for name, curve in zip(names, curves, strict=True)}
            for names, curves in parsed["group_curves"]
        ]

    def value(self, group: int, name: str, frame: float) -> float | None:
        curve = self.groups[group].get(name)
        return None if curve is None else sample(curve, frame)

    def triple(
        self, group: int, names: tuple[str, str, str], frame: float, identity: float
    ) -> tuple[float, float, float]:
        out = []
        for name in names:
            found = self.value(group, name, frame)
            out.append(identity if found is None else found)
        return (out[0], out[1], out[2])

    def translation_node(self, group: int, frame: float) -> list[list[float]]:
        return translation(self.triple(group, ("tx", "ty", "tz"), frame, 0.0))

    def full_node(
        self,
        group: int,
        frame: float,
        order: tuple[int, int, int],
        sign: float,
        rotate: bool,
    ) -> list[list[float]]:
        t = self.triple(group, ("tx", "ty", "tz"), frame, 0.0)
        degrees = self.triple(group, ("rx", "ry", "rz"), frame, 0.0)
        s = self.triple(group, ("sx", "sy", "sz"), frame, 1.0)
        radians = tuple(sign * angle * DEGREES_TO_RADIANS for angle in degrees)
        if not rotate:
            radians = (0.0, 0.0, 0.0)
        return multiply(
            multiply(scale(s), rotation(radians, order)),
            translation(t),
        )

    def frame_of(self, ticks: int) -> float:
        """0x00a112fb to 0x00a11326: the ticks are rounded to single precision first."""
        frame = f32(f32(float(ticks)) / TICKS_PER_FRAME)
        return f32(frame + self.parsed["offset_frames"])

    def points(
        self,
        camera: int,
        frame: float,
        order: tuple[int, int, int] = (0, 1, 2),
        sign: float = 1.0,
        rotate: bool = True,
    ) -> dict:
        """The chain A result, the eye, the interest point and the up point."""
        record = self.parsed["records"][camera]
        chain_a, chain_b, chain_c = record["chains"]
        world = IDENTITY
        for group in chain_a:
            world = multiply(self.full_node(group, frame, order, sign, rotate), world)
        eye = world
        for group in chain_b[:-1]:
            eye = multiply(self.translation_node(group, frame), eye)
        up = None
        if chain_b:
            up = multiply(self.translation_node(chain_b[-1], frame), eye)[3][:3]
        interest = world
        for group in chain_c:
            interest = multiply(self.translation_node(group, frame), interest)
        return {
            "world": world[3][:3],
            "eye": eye[3][:3],
            "interest": interest[3][:3],
            "up": up,
        }

    def selected_camera(self, frame: float) -> int:
        """The camera record the selector chooses: the step sample truncated toward zero."""
        selector = self.value(self.parsed["selector"], "tx", frame)
        if selector is None:
            raise ValueError("the selector group has no tx curve")
        index = math.trunc(selector)
        if not 0 <= index < len(self.parsed["records"]):
            raise ValueError("camera index outside the records")
        return index


def rotating_axes(rig: Rig, camera: int) -> int:
    """How many of rx, ry and rz are a nonzero curve in a chain A node of the camera."""
    record = rig.parsed["records"][camera]
    count = 0
    for axis in ("rx", "ry", "rz"):
        for group in record["chains"][0]:
            curve = rig.groups[group].get(axis)
            if curve is not None and not (curve[0] == "constant" and curve[1] == 0.0):
                count += 1
                break
    return count
