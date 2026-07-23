#pragma once

#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"

namespace ReEchoCollisionDebug
{
inline void DrawSphere(const UObject* WorldContext, const USphereComponent* Collision, const FColor Color)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!WorldContext || !Collision)
	{
		return;
	}
	DrawDebugSphere(WorldContext->GetWorld(),
	                Collision->GetComponentLocation(),
	                Collision->GetScaledSphereRadius(),
	                24,
	                Color,
	                false,
	                -1.0f,
	                1,
	                1.5f);
#endif
}

inline void DrawCapsule(const UObject* WorldContext, const UCapsuleComponent* Collision, const FColor Color)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!WorldContext || !Collision)
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

inline void DrawBounds(const UObject* WorldContext, const UPrimitiveComponent* Collision, const FColor Color)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!WorldContext || !Collision || !Collision->IsCollisionEnabled())
	{
		return;
	}
	DrawDebugBox(WorldContext->GetWorld(),
	             Collision->Bounds.Origin,
	             Collision->Bounds.BoxExtent,
	             Collision->GetComponentQuat(),
	             Color,
	             false,
	             -1.0f,
	             1,
	             1.5f);
#endif
}
}
