#pragma once

// Explicit precompiled header for the ReEchoAudio runtime module.
//
// UE 5.8 requires a secondary runtime module to provide an explicit PCH. Every
// translation unit force-includes this so CoreUObject (USTRUCT/UCLASS/
// GENERATED_BODY), the engine/world/audio types, and the core ticker the policy
// engine/service use are always available.
#include "CoreMinimal.h"
#include "CoreUObject.h"

#include "Engine/Engine.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Containers/Ticker.h"
