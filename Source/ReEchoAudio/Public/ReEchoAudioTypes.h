#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "Sound/SoundBase.h"
#include "ReEchoAudioTypes.generated.h"

// Logging category for the audio module. Kept minimal on purpose; the policy
// layer warns at most once per unknown event id so Shipping never floods.
REECHOAUDIO_API DECLARE_LOG_CATEGORY_EXTERN(LogReEchoAudio, Log, All);

// Forward declarations used by value-carrying structs below.
class UWorld;

/** Volume buses. Master is index 0 and factorises into every final volume. */
UENUM(BlueprintType)
enum class EReEchoAudioBus : uint8
{
	Master,
	Music,
	Ambience,
	CombatSfx,
	UiSfx
};

/** Independent long-loop state channels. */
UENUM(BlueprintType)
enum class EReEchoAudioChannel : uint8
{
	Music,
	Ambience
};

/** Whether an event definition is a one-shot or a looping state voice. */
UENUM(BlueprintType)
enum class EReEchoAudioEventType : uint8
{
	OneShot,
	Loop
};

/**
 * Pause policy for a voice.
 *  - PauseWithGame:   voice is paused when the game pauses (default).
 *  - ContinueOnPause: voice keeps playing even while paused (UI may opt in later).
 */
UENUM(BlueprintType)
enum class EReEchoAudioPausePolicy : uint8
{
	PauseWithGame,
	ContinueOnPause
};

/** Coarse source category of an event. Pure presentation metadata, never gameplay. */
UENUM(BlueprintType)
enum class EReEchoAudioSourceCategory : uint8
{
	Unspecified,
	Player,
	Enemy,
	Echo,
	Boss,
	Environment,
	Ui
};

/**
 * Stable, designer-facing audio event definition.
 *
 * The catalog populates the Sound soft pointer from designer-authored data and
 * preloads it asynchronously. Missing
 * or non-resident assets still degrade to
 * a safe no-op rather than a crash or synchronous load on the gameplay
 * thread.
 */
USTRUCT(BlueprintType)

struct REECHOAUDIO_API FReEchoAudioEventDefinition
{
	GENERATED_BODY()

	/** Stable event/state id. Must match a constant in ReEchoAudioEvents.h. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EventId;

	/** Optional stable variant id. Empty rows are the fallback for the event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName VariantId;

	/** Soft audio asset. Loaded lazily/non-blockingly; null is allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USoundBase> Sound;

	/** Which volume bus this event belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoAudioBus Bus = EReEchoAudioBus::CombatSfx;

	/** One-shot or looping state voice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoAudioEventType Type = EReEchoAudioEventType::OneShot;

	/** True to spatialise in the world at the request location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSpatial3D = false;

	/** Base volume multiplier for this event (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseVolume = 1.0f;

	/** Per-play pitch randomisation range (isolated from gameplay RNG). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PitchMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PitchMax = 1.0f;

	/** Minimum seconds between two plays of this event (0 = no cooldown). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CooldownSeconds = 0.0f;

	/** Max simultaneous voices (0 = unlimited). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxConcurrency = 0;

	/** Higher wins when over the concurrency budget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoAudioPausePolicy PausePolicy = EReEchoAudioPausePolicy::PauseWithGame;

	/** Optional attenuation falloff range (world units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttenuationMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttenuationMax = 0.0f;
};

/**
 * Semantic request published by gameplay. Gameplay must NOT pass USoundBase,
 * UAudioComponent, asset paths, SoundClass/SoundMix, or any play duration here.
 */
USTRUCT(BlueprintType)

struct REECHOAUDIO_API FReEchoAudioEventRequest
{
	GENERATED_BODY()

	/** Stable event id (see ReEchoAudioEvents.h). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EventId;

	/** Optional world-space location for 3D events. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WorldLocation = FVector::ZeroVector;

	/** Optional coarse source category (pure presentation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EReEchoAudioSourceCategory SourceCategory = EReEchoAudioSourceCategory::Unspecified;

	/** Optional variant (e.g. weapon/enemy variant) resolved later by catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName VariantId;

	/** Pure presentation intensity (0..n), e.g. scales volume within bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Intensity = 1.0f;
};

/**
 * Resolved play instruction handed from the policy engine to the backend.
 * Contains the final volume/pitch already computed by the module.
 */
struct REECHOAUDIO_API FReEchoAudioPlayCommand
{
	FName EventId;
	TSoftObjectPtr<USoundBase> Sound;
	EReEchoAudioBus Bus = EReEchoAudioBus::CombatSfx;
	bool bSpatial3D = false;
	FVector Location = FVector::ZeroVector;
	float Volume = 1.0f;
	float Pitch = 1.0f;
	EReEchoAudioPausePolicy PausePolicy = EReEchoAudioPausePolicy::PauseWithGame;
	float FadeInSeconds = 0.0f;
	float AttenuationMin = 0.0f;
	float AttenuationMax = 0.0f;
	FName VariantId;
	EReEchoAudioSourceCategory SourceCategory = EReEchoAudioSourceCategory::Unspecified;
	UWorld* World = nullptr;
};
