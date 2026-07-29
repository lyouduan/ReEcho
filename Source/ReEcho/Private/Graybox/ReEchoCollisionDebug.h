#pragma once

#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

namespace ReEchoCollisionDebug
{
inline bool IsEnabled()
{
	static const TAutoConsoleVariable<int32> DebugCollision(
	    TEXT("ReEcho.DebugCollision"), 0, TEXT("Draw ReEcho collision volumes. 0: disabled, 1: enabled."), ECVF_Cheat);
	return DebugCollision.GetValueOnGameThread() != 0;
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
