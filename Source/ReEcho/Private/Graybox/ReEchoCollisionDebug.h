#pragma once

#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Presentation/Animation2D/ReEcho2DCollisionDebug.h"

namespace ReEchoCollisionDebug
{
inline bool IsEnabled()
{
	return ReEcho2DCollisionDebug::GetLevel() >= 1;
}

inline void DrawCapsule(const UObject* WorldContext, const UCapsuleComponent* Collision, const FColor Color)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!IsEnabled() || !WorldContext || !Collision)
	{
		return;
	}
	DrawDebugCapsule(WorldContext->GetWorld(),
	                 Collision->GetComponentLocation(),
	                 Collision->GetScaledCapsuleHalfHeight(),
	                 Collision->GetScaledCapsuleRadius(),
	                 Collision->GetComponentQuat(),
	                 Color,
	                 false,
	                 -1.0f,
	                 1,
	                 1.5f);
#endif
}

}
