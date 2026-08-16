# ReEcho diagnostic tone provenance

- Purpose: Plan34 technical verification of Master/UI SFX volume and mute behavior.
- Source: repository-local deterministic generator `scripts/audio/generate_diagnostic_tone.py`.
- Signal: 440 Hz sine, 1 second, mono PCM16, 48 kHz, amplitude 0.20, 10 ms edge fades.
- License/origin: generated mathematically by the project script; no third-party recording or media is used.
- Runtime asset: `/Game/ReEcho/Audio/Diagnostics/ReEchoDiagTone.ReEchoDiagTone`.
- Catalog binding: `UI.Error` only. This is replaceable technical media, not approved production sound design.
- Reproduction: run the generator, then execute `scripts/audio/import_diagnostic_tone.py` inside Unreal Editor Python.

The generated `.wav` and imported `.uasset` are intentionally not produced by the coding executor. The user imports and aurally validates them in the Editor/PIE environment.
