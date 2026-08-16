#!/usr/bin/env python3
"""Generate the replaceable Plan34 440 Hz diagnostic WAV deterministically."""
from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "Design" / "Audio" / "Generated" / "ReEchoDiagTone.wav"
SAMPLE_RATE = 48_000
DURATION_SECONDS = 1.0
FREQUENCY_HZ = 440.0
AMPLITUDE = 0.20


def main() -> int:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    frame_count = int(SAMPLE_RATE * DURATION_SECONDS)
    with wave.open(str(OUTPUT), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        for frame in range(frame_count):
            # Short deterministic fade avoids clicks without production mastering.
            time = frame / SAMPLE_RATE
            edge = min(time / 0.01, (DURATION_SECONDS - time) / 0.01, 1.0)
            sample = int(32767 * AMPLITUDE * max(0.0, edge) * math.sin(2.0 * math.pi * FREQUENCY_HZ * time))
            wav.writeframesraw(struct.pack("<h", sample))
    print(f"generated {OUTPUT} ({SAMPLE_RATE} Hz mono PCM16, {FREQUENCY_HZ} Hz, {DURATION_SECONDS}s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
