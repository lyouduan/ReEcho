#!/usr/bin/env python3
"""Generate deterministic, replaceable baseline one-shot audio for Plan46."""
from __future__ import annotations

import argparse
import hashlib
import io
import math
import random
import struct
import wave
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUTPUT_ROOT = ROOT / "Design" / "Audio" / "Generated"
SAMPLE_RATE = 48_000
MASTER_PEAK = 0.72


@dataclass(frozen=True)
class Tone:
    start_hz: float
    end_hz: float
    amplitude: float
    decay: float
    start_seconds: float = 0.0
    waveform: str = "sine"


@dataclass(frozen=True)
class Pulse:
    start_seconds: float
    frequency_hz: float
    amplitude: float
    decay: float


@dataclass(frozen=True)
class Recipe:
    relative_path: str
    duration: float
    tones: tuple[Tone, ...]
    noise_amplitude: float = 0.0
    noise_decay: float = 8.0
    noise_smoothing: float = 0.15
    pulses: tuple[Pulse, ...] = ()
    echo_seconds: float = 0.0
    echo_gain: float = 0.0


RECIPES: dict[str, Recipe] = {
    "UI.Hover": Recipe("UI/UI_Hover.wav", 0.14, (Tone(1050, 1550, 0.50, 11.0),), 0.04, 18.0),
    "UI.Confirm": Recipe("UI/UI_Confirm.wav", 0.34, (Tone(620, 1050, 0.46, 4.8), Tone(1240, 1680, 0.22, 7.0, 0.08)), 0.03, 12.0, pulses=(Pulse(0.0, 2100, 0.20, 55.0),)),
    "UI.Cancel": Recipe("UI/UI_Cancel.wav", 0.36, (Tone(620, 260, 0.45, 5.0), Tone(930, 430, 0.18, 7.0, 0.05)), 0.03, 12.0),
    "UI.Error": Recipe("UI/UI_Error.wav", 0.48, (Tone(250, 185, 0.46, 3.8, 0.0, "triangle"), Tone(245, 175, 0.38, 5.0, 0.18, "triangle")), 0.05, 10.0, pulses=(Pulse(0.0, 900, 0.14, 34.0), Pulse(0.18, 760, 0.12, 34.0))),
    "UI.Purchase": Recipe("UI/UI_Purchase.wav", 0.58, (Tone(980, 1320, 0.32, 3.5), Tone(1480, 2050, 0.26, 4.5, 0.09), Tone(2050, 2450, 0.15, 6.0, 0.18)), 0.025, 14.0, pulses=(Pulse(0.0, 2600, 0.18, 48.0),), echo_seconds=0.10, echo_gain=0.18),
    "UI.CardSelect": Recipe("UI/UI_CardSelect.wav", 0.52, (Tone(520, 1180, 0.32, 4.0, 0.08), Tone(1100, 1640, 0.22, 5.0, 0.16)), 0.12, 9.0, 0.08, (Pulse(0.22, 1800, 0.18, 40.0),), 0.09, 0.16),
    "Combat.Attack": Recipe("Combat/Combat_Attack.wav", 0.46, (Tone(340, 105, 0.24, 5.0, 0.06, "saw"), Tone(980, 420, 0.15, 8.0, 0.12)), 0.34, 6.0, 0.06),
    "Combat.Hit": Recipe("Combat/Combat_Hit.wav", 0.42, (Tone(125, 72, 0.55, 7.0, 0.0, "triangle"), Tone(680, 260, 0.16, 12.0)), 0.28, 11.0, 0.16, (Pulse(0.0, 1200, 0.24, 60.0),)),
    "Combat.Hurt": Recipe("Combat/Combat_Hurt.wav", 0.58, (Tone(180, 82, 0.50, 5.5, 0.0, "triangle"), Tone(420, 160, 0.16, 7.0)), 0.22, 9.0, 0.22, (Pulse(0.0, 760, 0.16, 45.0),)),
    "Combat.Death": Recipe("Combat/Combat_Death.wav", 1.55, (Tone(170, 42, 0.52, 2.2, 0.0, "triangle"), Tone(520, 105, 0.22, 3.5, 0.10, "saw")), 0.24, 3.5, 0.14, (Pulse(0.0, 620, 0.18, 38.0),), 0.18, 0.20),
    "Enemy.Spawn": Recipe("Enemy/Enemy_Spawn.wav", 0.90, (Tone(145, 520, 0.40, 2.3, 0.06, "triangle"), Tone(510, 880, 0.18, 3.8, 0.20)), 0.20, 4.2, 0.08, (Pulse(0.58, 180, 0.28, 32.0),), 0.11, 0.16),
    "Enemy.Death": Recipe("Enemy/Enemy_Death.wav", 1.12, (Tone(230, 55, 0.46, 3.2, 0.0, "triangle"), Tone(690, 130, 0.18, 4.8, 0.08, "saw")), 0.30, 4.8, 0.12, (Pulse(0.0, 520, 0.18, 40.0),), 0.13, 0.18),
    "Boss.Death": Recipe("Boss/Boss_Death.wav", 4.60, (Tone(105, 28, 0.55, 0.8, 0.0, "triangle"), Tone(330, 46, 0.21, 1.1, 0.18, "saw"), Tone(760, 120, 0.11, 1.5, 0.70)), 0.32, 1.2, 0.10, (Pulse(0.0, 90, 0.28, 12.0), Pulse(1.20, 72, 0.30, 8.0), Pulse(2.55, 52, 0.34, 5.0)), 0.28, 0.22),
    "CameraMove": Recipe("Flow/CameraMove.wav", 0.42, (Tone(310, 470, 0.12, 5.0, 0.05),), 0.25, 7.0, 0.05, (), 0.08, 0.12),
    "Revive": Recipe("Flow/Revive.wav", 1.38, (Tone(260, 520, 0.34, 2.0, 0.02), Tone(520, 1040, 0.24, 2.5, 0.20), Tone(1040, 1560, 0.14, 3.0, 0.45)), 0.08, 5.0, 0.12, (Pulse(0.92, 1800, 0.14, 24.0),), 0.12, 0.18),
}


def _wave_sample(kind: str, phase: float) -> float:
    cycle = phase / (2.0 * math.pi)
    if kind == "triangle":
        return 2.0 * abs(2.0 * (cycle - math.floor(cycle + 0.5))) - 1.0
    if kind == "saw":
        return 2.0 * (cycle - math.floor(cycle + 0.5))
    return math.sin(phase)


def _edge_envelope(time: float, duration: float, attack: float = 0.006, release: float = 0.025) -> float:
    return max(0.0, min(1.0, time / attack, (duration - time) / release))


def render_wav(event_id: str, recipe: Recipe) -> bytes:
    frame_count = int(round(recipe.duration * SAMPLE_RATE))
    samples = [0.0] * frame_count
    seed = int.from_bytes(hashlib.sha256(event_id.encode("utf-8")).digest()[:8], "little")
    rng = random.Random(seed)

    for tone in recipe.tones:
        start_frame = int(round(tone.start_seconds * SAMPLE_RATE))
        phase = 0.0
        active_duration = max(1.0 / SAMPLE_RATE, recipe.duration - tone.start_seconds)
        for frame in range(start_frame, frame_count):
            local_time = (frame - start_frame) / SAMPLE_RATE
            ratio = min(1.0, local_time / active_duration)
            frequency = tone.start_hz + (tone.end_hz - tone.start_hz) * ratio
            phase += 2.0 * math.pi * frequency / SAMPLE_RATE
            envelope = _edge_envelope(local_time, active_duration) * math.exp(-tone.decay * local_time)
            samples[frame] += tone.amplitude * envelope * _wave_sample(tone.waveform, phase)

    if recipe.noise_amplitude > 0.0:
        smoothed = 0.0
        for frame in range(frame_count):
            time = frame / SAMPLE_RATE
            raw = rng.uniform(-1.0, 1.0)
            smoothed += recipe.noise_smoothing * (raw - smoothed)
            envelope = _edge_envelope(time, recipe.duration) * math.exp(-recipe.noise_decay * time)
            samples[frame] += recipe.noise_amplitude * envelope * smoothed

    for pulse in recipe.pulses:
        start_frame = int(round(pulse.start_seconds * SAMPLE_RATE))
        phase = 0.0
        for frame in range(start_frame, frame_count):
            local_time = (frame - start_frame) / SAMPLE_RATE
            phase += 2.0 * math.pi * pulse.frequency_hz / SAMPLE_RATE
            envelope = math.exp(-pulse.decay * local_time)
            samples[frame] += pulse.amplitude * envelope * (0.72 * math.sin(phase) + 0.28 * rng.uniform(-1.0, 1.0))

    if recipe.echo_seconds > 0.0 and recipe.echo_gain > 0.0:
        delay = int(round(recipe.echo_seconds * SAMPLE_RATE))
        dry = samples.copy()
        for frame in range(delay, frame_count):
            samples[frame] += recipe.echo_gain * dry[frame - delay]

    peak = max(max(abs(sample) for sample in samples), 1.0e-9)
    gain = MASTER_PEAK / peak
    pcm = bytearray()
    for frame, sample in enumerate(samples):
        time = frame / SAMPLE_RATE
        safe_sample = sample * gain * _edge_envelope(time, recipe.duration)
        pcm.extend(struct.pack("<h", int(round(max(-1.0, min(1.0, safe_sample)) * 32767.0))))

    output = io.BytesIO()
    with wave.open(output, "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(bytes(pcm))
    return output.getvalue()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="verify tracked outputs match deterministic generation")
    args = parser.parse_args()

    failures: list[str] = []
    for event_id, recipe in RECIPES.items():
        output_path = OUTPUT_ROOT / recipe.relative_path
        expected = render_wav(event_id, recipe)
        if args.check:
            if not output_path.is_file() or output_path.read_bytes() != expected:
                failures.append(str(output_path.relative_to(ROOT)))
            continue
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_bytes(expected)
        print(f"generated {event_id}: {output_path.relative_to(ROOT)} ({recipe.duration:.2f}s mono PCM16)")

    if failures:
        print("generated audio drift: " + ", ".join(failures))
        return 1
    if args.check:
        print(f"[PASS] {len(RECIPES)} deterministic one-shot WAV files match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
