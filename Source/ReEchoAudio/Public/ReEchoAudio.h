#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Standalone runtime module for all ReEcho audio behaviour.
 *
 * Dependency contract (see plans/33-audio-runtime-module-foundation.md):
 *   ReEcho        -> ReEchoAudio   (gameplay publishes semantic intent)
 *   ReEchoAudio   -/-> ReEcho      (must never include gameplay headers)
 *
 * This module owns audio resource/playback, music/ambience state crossfades,
 * 2D/3D playback, volume buses, per-event volume/pitch/spatial/pause policy,
 * cooldown, concurrency, priority, and safe degradation on missing assets.
 * Audio failures are always non-blocking no-ops that never change gameplay.
 */
class FReEchoAudioModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};
