"""Build deterministic derived audio required by the formal Plan114 handoff."""

from __future__ import annotations

import argparse
from array import array
import hashlib
import io
from pathlib import Path
import sys
import wave


ROOT = Path(__file__).resolve().parents[2]
ENEMY_SPAWN_SOURCE = ROOT / "Design/Audio/Source/Formal/Enemy/Enemy_Spawn_Source.wav"
ENEMY_SPAWN_OUTPUT = ROOT / "Design/Audio/Derived/Enemy/Enemy_Spawn.wav"
ENEMY_SPAWN_SOURCE_SHA256 = "c6fd22f3536f66a852de21ae620a0922f8872e8c24a49d292c123eaa987d9703"

SPATIAL_WAVS = {
    "Combat.Hurt": (
        ROOT / "Design/Audio/Source/Formal/Combat/Combat_Hurt.wav",
        ROOT / "Design/Audio/Derived/Combat/Combat_Hurt.wav",
    ),
    "Combat.Death": (
        ROOT / "Design/Audio/Source/Formal/Combat/Combat_Death.wav",
        ROOT / "Design/Audio/Derived/Combat/Combat_Death.wav",
    ),
    "Boss.Death": (
        ROOT / "Design/Audio/Source/Formal/Boss/Boss_Death.wav",
        ROOT / "Design/Audio/Derived/Boss/Boss_Death.wav",
    ),
    "Enemy.Death": (
        ROOT / "Design/Audio/Decoded/Enemy/Enemy_Death.wav",
        ROOT / "Design/Audio/Derived/Enemy/Enemy_Death.wav",
    ),
    "Combat.Attack/W_J_01": (
        ROOT / "Design/Audio/Decoded/Variants/CombatAttack/W_J_01.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatAttack/W_J_01.wav",
    ),
    "Combat.Attack/W_J_04": (
        ROOT / "Design/Audio/Source/Formal/Variants/CombatAttack/W_J_04.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatAttack/W_J_04.wav",
    ),
    "Combat.Attack/W_J_08": (
        ROOT / "Design/Audio/Decoded/Variants/CombatAttack/W_J_08.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatAttack/W_J_08.wav",
    ),
    "Combat.Attack/W_J_09": (
        ROOT / "Design/Audio/Source/Formal/Variants/CombatAttack/W_J_09.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatAttack/W_J_09.wav",
    ),
    "Combat.Hit/Flame": (
        ROOT / "Design/Audio/Decoded/Variants/CombatHit/Flame.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatHit/Flame.wav",
    ),
    "Combat.Hit/Lightning": (
        ROOT / "Design/Audio/Source/Formal/Variants/CombatHit/Lightning.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatHit/Lightning.wav",
    ),
    "Combat.Hit/Grass": (
        ROOT / "Design/Audio/Source/Formal/Variants/CombatHit/Grass.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatHit/Grass.wav",
    ),
    "Combat.Hit/Water": (
        ROOT / "Design/Audio/Source/Formal/Variants/CombatHit/Water.wav",
        ROOT / "Design/Audio/Derived/Variants/CombatHit/Water.wav",
    ),
}


def read_pcm16(source_bytes: bytes, label: str) -> tuple[int, int, array]:
    with wave.open(io.BytesIO(source_bytes), "rb") as source:
        channels = source.getnchannels()
        sample_width = source.getsampwidth()
        sample_rate = source.getframerate()
        frame_count = source.getnframes()
        compression = source.getcomptype()
        frames = source.readframes(frame_count)

    if channels not in (1, 2) or sample_width != 2 or compression != "NONE":
        raise RuntimeError(
            f"{label} must remain mono/stereo PCM16 "
            f"(channels={channels}, sample_width={sample_width}, compression={compression})"
        )

    samples = array("h")
    samples.frombytes(frames)
    if sys.byteorder != "little":
        samples.byteswap()
    return channels, sample_rate, samples


def encode_mono_pcm16(samples: array, sample_rate: int) -> bytes:
    if sys.byteorder != "little":
        samples.byteswap()
    output = io.BytesIO()
    with wave.open(output, "wb") as target:
        target.setnchannels(1)
        target.setsampwidth(2)
        target.setframerate(sample_rate)
        target.writeframes(samples.tobytes())
    return output.getvalue()


def downmix_to_mono(source_bytes: bytes, label: str) -> bytes:
    channels, sample_rate, samples = read_pcm16(source_bytes, label)
    if channels == 2:
        samples = array(
            "h",
            ((samples[index] + samples[index + 1]) // 2 for index in range(0, len(samples), 2)),
        )
    return encode_mono_pcm16(samples, sample_rate)


def build_enemy_spawn() -> bytes:
    source_bytes = ENEMY_SPAWN_SOURCE.read_bytes()
    actual_hash = hashlib.sha256(source_bytes).hexdigest()
    if actual_hash != ENEMY_SPAWN_SOURCE_SHA256:
        raise RuntimeError(
            f"Enemy.Spawn source hash mismatch: expected {ENEMY_SPAWN_SOURCE_SHA256}, found {actual_hash}"
        )

    channels, sample_rate, samples = read_pcm16(source_bytes, "Enemy.Spawn source")
    if channels != 2:
        raise RuntimeError(f"Enemy.Spawn source must remain stereo PCM16 (channels={channels})")

    frame_count = len(samples) // channels
    first_second_half_sample = (frame_count // 2) * channels
    second_half = samples[first_second_half_sample:]
    mono = array(
        "h",
        ((second_half[index] + second_half[index + 1]) // 2 for index in range(0, len(second_half), 2)),
    )
    return encode_mono_pcm16(mono, sample_rate)


def describe_wav(payload: bytes) -> str:
    with wave.open(io.BytesIO(payload), "rb") as source:
        duration = source.getnframes() / source.getframerate()
        return f"{duration:.3f}s mono PCM16 {source.getframerate() // 1000}kHz"


def write_or_check(output: Path, expected: bytes, check: bool, label: str) -> None:
    if check:
        if not output.is_file() or output.read_bytes() != expected:
            raise SystemExit(f"{label} derived WAV is missing or stale; run prepare_formal_audio.py")
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(expected)

    digest = hashlib.sha256(expected).hexdigest()
    print(f"[PASS] {label} derived: {describe_wav(expected)} sha256={digest}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="verify the tracked derived WAV byte-for-byte")
    args = parser.parse_args()

    write_or_check(ENEMY_SPAWN_OUTPUT, build_enemy_spawn(), args.check, "Enemy.Spawn")
    for label, (source, output) in SPATIAL_WAVS.items():
        write_or_check(output, downmix_to_mono(source.read_bytes(), label), args.check, label)


if __name__ == "__main__":
    main()
