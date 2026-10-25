#!/usr/bin/env python3
"""Reference math of the Qix emitter family.

Each function restates a rule of docs/resource/vfx-generate-emitters.md over plain
Python numbers. Draws are passed in, so a caller owns the random source. The client
stores floats and mixes single and double steps. This module keeps doubles and
rounds to float32 only where the page names a store, so results match the client to
float tolerance and not bit for bit.
"""

from __future__ import annotations

import math
import struct

TICKS_PER_FRAME = 5000.0
MT_SIZE = 624
MT_SHIFT = 397


def f32(value: float) -> float:
    try:
        return struct.unpack("<f", struct.pack("<f", value))[0]
    except OverflowError:
        return math.copysign(math.inf, value)


def signed32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - (1 << 32) if value & 0x80000000 else value


class ClientMt:
    """MT19937 as the client indexes it.

    The state counter counts down from 623 after a refill and the output index is
    624 minus the counter before the decrement. The first output after a refill is
    therefore word 1, the 624th tempers the pointer word at object +0x9c0 (the
    address of the counter at +0x9c4) and a refill happens on the next call, which
    outputs word 0. pointer_word stands in for that address.
    """

    def __init__(self, seed: int, pointer_word: int = 0) -> None:
        self.state = [seed & 0xFFFFFFFF]
        for index in range(1, MT_SIZE):
            previous = self.state[-1]
            self.state.append(
                (1812433253 * (previous ^ (previous >> 30)) + index) & 0xFFFFFFFF
            )
        self.pointer_word = pointer_word & 0xFFFFFFFF
        self.counter = 0
        self.refill()

    def refill(self) -> None:
        state = self.state
        for k in range(MT_SIZE):
            high = state[k] & 0x80000000
            low = state[(k + 1) % MT_SIZE] & 0x7FFFFFFF
            mixed = high | low
            state[k] = state[(k + MT_SHIFT) % MT_SIZE] ^ (mixed >> 1)
            if mixed & 1:
                state[k] ^= 0x9908B0DF
        self.counter = MT_SIZE - 1

    @staticmethod
    def temper(word: int) -> int:
        word ^= word >> 11
        word ^= (word << 7) & 0x9D2C5680
        word ^= (word << 15) & 0xEFC60000
        word ^= word >> 18
        return word & 0xFFFFFFFF

    def next_u32(self) -> int:
        old = self.counter
        self.counter -= 1
        if old == -1:
            self.refill()
            old = MT_SIZE
        index = MT_SIZE - old
        word = self.pointer_word if index == MT_SIZE else self.state[index]
        return self.temper(word)


def unit_draw(word: int) -> float:
    """[0, 1] float from one output word. The scale is 1 / 4294967295."""
    return f32((word & 0xFFFFFFFF) / 4294967295.0)


def signed_draw(word: int) -> float:
    """[-1, 1) float from one output word, through a float store of the int."""
    return f32(f32(float(signed32(word))) * 2.0**-31)


def polynomial(coefficients, degree: int, x: float, carry: float = 0.0) -> float:
    """One component of a polynomial graph segment.

    Degree 0 keeps the constant term. The terms of degree 2 and above carry 0.5.
    """
    if degree == 0:
        return f32(coefficients[0] + carry)
    total = coefficients[1] * x + coefficients[0] + carry
    for power in range(2, degree + 1):
        total += coefficients[power] * 0.5 * x**power
    return f32(total)


def graph_segment(starts, ticks: int) -> int:
    """Segment index the client reaches by stepping while the next start has passed."""
    index = 0
    while index < len(starts) - 1 and ticks >= signed32(starts[index + 1]):
        index += 1
    return index


def graph_value(starts, segments, ticks: int, component: int = 0) -> float:
    """Value of one component of a polynomial graph binding at a unit time in ticks."""
    index = graph_segment(starts, ticks)
    degree, components, coefficients = segments[index]
    x = (ticks - signed32(starts[index])) / TICKS_PER_FRAME
    row = coefficients[(degree + 1) * component : (degree + 1) * (component + 1)]
    assert component < components
    return polynomial(row, degree, x)


class Genesis:
    """Event counter of one Genesis binding.

    The polynomial is the cumulative event count. Each step emits the integer part
    of the peak count in the step minus the count already emitted. gated stands for
    a nonzero dword at [[value +0x04] +0x10], which suppresses every event.
    """

    def __init__(self, starts, segments, gated: bool = False) -> None:
        self.starts = [signed32(item) for item in starts]
        self.segments = segments
        self.gated = gated
        self.index = None
        self.emitted = 0.0

    def burst(self, segment) -> int:
        degree, _, coefficients = segment
        if degree != 0 or self.gated:
            return 0
        count = int(coefficients[0] - self.emitted)
        if count <= 0:
            return 0
        self.emitted += count
        return count

    def peak(self, segment, x: float, span: float) -> int:
        degree, _, c = segment
        if degree == 0 or self.gated:
            return 0
        start = x - span
        if degree == 1:
            at = max(start, 0.0) if c[1] <= 0 else x
            peak = c[0] + c[1] * at
        elif c[2] * 0.5 == 0:
            peak = c[0] + c[1] * (x if c[1] >= 0 else start)
        else:
            half = c[2] * 0.5

            def count(t: float) -> float:
                return c[0] + c[1] * t + half * t * t

            vertex = -(c[1] / (half * 2.0))
            peak = count(start)
            if start <= vertex <= x and count(vertex) >= peak:
                peak = count(vertex)
            elif count(start) < count(x):
                peak = count(x)
        if peak < self.emitted:
            return 0
        whole = int(peak - self.emitted)
        self.emitted += whole
        return whole

    def step(self, delta: int, ticks: int) -> int:
        """Events emitted by one graph update of delta ticks that ends at ticks."""
        total = 0
        frames = delta / TICKS_PER_FRAME
        if self.index is None:
            self.index = 0
            total += self.burst(self.segments[0])
        start = 0.0
        index = self.index
        while index < len(self.starts) - 1 and ticks >= self.starts[index + 1]:
            following = self.starts[index + 1]
            duration = (following - self.starts[index]) / TICKS_PER_FRAME
            end = ((following - ticks) + delta) / TICKS_PER_FRAME
            total += self.peak(self.segments[index], duration, end - start)
            total += self.burst(self.segments[index + 1])
            start = end
            index += 1
        x = (ticks - self.starts[index]) / TICKS_PER_FRAME
        total += self.peak(self.segments[index], x, frames - start)
        self.index = index
        return total


def spawn_probability(base: float, spread: float, draw: int) -> float:
    """Spawn chance of one event from the first draw word."""
    return f32(f32(spread * signed_draw(draw)) + base)


def distance_pass(near: float, far: float, floor: float, distance: float, draw: int):
    """Camera distance thinning. Returns True when the spawn is kept."""
    if floor == 1.0 or distance <= near:
        return True
    chance = floor
    if distance < far:
        chance = 1.0 - ((distance - near) / (far - near)) * (1.0 - floor)
    return not chance < unit_draw(draw)


def axis_value(axis_on: int, axis_signed: int, draw: int) -> float:
    """One axis of the area random vector from the two flag bits of that axis."""
    unit = unit_draw(draw)
    if axis_on:
        return unit * 2.0 - 1.0 if axis_signed else unit
    return -unit if axis_signed else 0.0


def axis_vector(flags: int, draws) -> tuple[float, float, float]:
    """Random vector of an area record from flag bits 1 to 6 and three draw words."""
    return tuple(
        axis_value((flags >> (1 + 2 * axis)) & 1, (flags >> (2 + 2 * axis)) & 1, draw)
        for axis, draw in enumerate(draws)
    )


def normalize(vector):
    length = math.sqrt(sum(item * item for item in vector))
    if not length > 0.0:
        return tuple(vector)
    return tuple(item / length for item in vector)


def sphere_sample(flags: int, radius: float, jitter: float, draws):
    """Area kind 1. draws: three axis words, the jitter word, the volume word."""
    vector = axis_vector(flags, draws[:3])
    reach = radius + jitter * unit_draw(draws[3])
    if not flags & 1:
        reach *= unit_draw(draws[4])
    position = tuple(item * reach for item in normalize(vector))
    return position, normalize(position)


def box_sample(flags: int, extent, spread, draws):
    """Area kind 2. draws: three axis words, then one word per axis."""
    vector = list(axis_vector(flags, draws[:3]))
    largest, best = 0, vector[0]
    for axis in (1, 2):
        if abs(vector[axis]) >= best:
            largest, best = axis, vector[axis]
    if flags & 1:
        vector[largest] = 1.0 if vector[largest] > 0.0 else -1.0
    position = tuple(
        (unit_draw(draws[3 + axis]) * spread[axis] + extent[axis]) * vector[axis]
        for axis in range(3)
    )
    return position, normalize(position)


def cylinder_sample(flags: int, radius: float, height: float, jitters, draws):
    """Area kind 3. jitters are the radius and height jitter amplitudes."""
    vector = axis_vector(flags, draws[:3])
    angle = math.atan2(vector[0], vector[2])
    reach = unit_draw(draws[3]) * radius
    span = height + jitters[1] * unit_draw(draws[4])
    side = vector[1]
    if flags & 1:
        if math.hypot(vector[0], vector[2]) < abs(vector[1]):
            side = 1.0 if vector[1] >= 0.0 else -1.0
        else:
            reach = radius
    reach += jitters[0] * unit_draw(draws[5])
    position = (math.cos(angle) * reach, span * side, math.sin(angle) * reach)
    return position, normalize((position[0], 0.0, position[2]))


def generated_position(origin, direction, speed, accel, ticks, parent=(0, 0, 0)):
    """Position3DGenerated output without the drag mode."""
    t = ticks / TICKS_PER_FRAME
    return tuple(
        parent[i]
        + origin[i]
        + direction[i] * speed * t
        + direction[i] * accel * t * t * 0.5
        for i in range(3)
    )


class DragPosition:
    """Position3DGenerated in the drag mode. The damping applies once per update."""

    def __init__(self, origin, direction, speed, damping) -> None:
        self.velocity = tuple(item * speed for item in direction)
        self.origin = tuple(origin)
        self.previous = tuple(origin)
        self.sum = tuple(origin)
        self.damping = damping
        self.factor = 1.0
        self.ticks = 0

    def update(self, delta: int, parent=(0, 0, 0)):
        self.ticks += delta
        t = self.ticks / TICKS_PER_FRAME
        line = tuple(self.origin[i] + self.velocity[i] * t for i in range(3))
        self.sum = tuple(
            self.sum[i] + (line[i] - self.previous[i]) * self.factor for i in range(3)
        )
        self.previous = line
        self.factor *= self.damping
        return tuple(parent[i] + self.sum[i] for i in range(3))


def generated_rotation(origin, speed, accel, ticks, parent=(0, 0, 0)):
    """Angle3DGenerated output."""
    t = ticks / TICKS_PER_FRAME
    return tuple(
        parent[i] + speed[i] * t + accel[i] * 0.5 * t * t + origin[i] for i in range(3)
    )


def generated_scale(origin, speed, accel, ticks, parent=(1, 1, 1)):
    """Scale3DGenerated output. The parent multiplies."""
    t = ticks / TICKS_PER_FRAME
    return tuple(
        parent[i] * (speed[i] * t + accel[i] * 0.5 * t * t + origin[i])
        for i in range(3)
    )


def line_lifetime_ticks(base: int, spread: int, draw_index: int) -> int:
    """Lifetime of a ManyGenerate line from an integer draw in [0, spread]."""
    assert 0 <= draw_index <= spread
    return (base + draw_index) * 5000


def particle_step(position, velocity, accel, ticks: int):
    """One kinematic step of a ManyGenerate particle."""
    dt = ticks / TICKS_PER_FRAME
    half = dt * 0.5
    moved = tuple(position[i] + (velocity[i] + accel[i] * half) * dt for i in range(3))
    faster = tuple(velocity[i] + accel[i] * dt for i in range(3))
    return moved, faster


def emission_init(record: dict, direction, draws):
    """ManyGenerateMotionEmission start state from 14 signed draw words.

    record holds speed, drag, velocity and accel as (base, spread) pairs where the
    last two are per axis triples. The first two words feed speed and drag, then
    each axis takes a jitter word and a sign word, velocity first.
    """
    words = [signed_draw(word) for word in draws]
    speed = record["speed"][0] + record["speed"][1] * words[0]
    drag = record["drag"][0] + record["drag"][1] * words[1]
    out = []
    for key in ("velocity", "accel"):
        base, spread = record[key]
        row = []
        for axis in range(3):
            jitter = spread[axis] * (words[2 + len(out) * 6 + axis * 2] * 0.5)
            if words[3 + len(out) * 6 + axis * 2] * 3.0 < 1.0:
                jitter *= -1.0
            row.append(base[axis] + jitter)
        out.append(row)
    velocity, accel = out
    if drag == 1.0:
        velocity = [velocity[axis] + direction[axis] * speed for axis in range(3)]
    return velocity, accel, drag
