#pragma once

#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

namespace ReEchoBillboardScreenScale
{
inline float Calculate(const UObject* WorldContext, const FVector& WorldLocation)
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContext, 0);
	if (!Camera)
	{
		return 1.0f;
	}

	constexpr float ReferenceDepth = 1000.0f;
	const FVector CameraToVisual = WorldLocation - Camera->GetCameraLocation();
	const float ViewDepth = FVector::DotProduct(CameraToVisual, Camera->GetCameraRotation().Vector());
	return FMath::Clamp(ViewDepth / ReferenceDepth, 0.35f, 3.0f);
}
}
