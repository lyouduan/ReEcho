#pragma once

#include "Camera/PlayerCameraManager.h"
#include "Components/BillboardComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"

namespace ReEchoBillboardDebug
{
inline void DrawBounds(const UObject* WorldContext, const UBillboardComponent* Billboard, const FColor Color)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!WorldContext || !Billboard || !Billboard->IsVisible() || !Billboard->Sprite)
	{
		return;
	}

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContext, 0);
	if (!Camera)
	{
		return;
	}

	const FRotationMatrix CameraRotation(Camera->GetCameraRotation());
	const FVector Right = CameraRotation.GetUnitAxis(EAxis::Y);
	const FVector Up = CameraRotation.GetUnitAxis(EAxis::Z);
	const FVector Scale = Billboard->GetComponentScale();
	const float HalfWidth = Billboard->Sprite->GetSizeX() * Scale.X * 0.5f + 2.0f;
	const float HalfHeight = Billboard->Sprite->GetSizeY() * Scale.Z * 0.5f + 2.0f;
	const FVector Center = Billboard->GetComponentLocation();

	const FVector TopLeft = Center - Right * HalfWidth + Up * HalfHeight;
	const FVector TopRight = Center + Right * HalfWidth + Up * HalfHeight;
	const FVector BottomRight = Center + Right * HalfWidth - Up * HalfHeight;
	const FVector BottomLeft = Center - Right * HalfWidth - Up * HalfHeight;
	constexpr float LineThickness = 1.5f;
	DrawDebugLine(WorldContext->GetWorld(), TopLeft, TopRight, Color, false, -1.0f, 1, LineThickness);
	DrawDebugLine(WorldContext->GetWorld(), TopRight, BottomRight, Color, false, -1.0f, 1, LineThickness);
	DrawDebugLine(WorldContext->GetWorld(), BottomRight, BottomLeft, Color, false, -1.0f, 1, LineThickness);
	DrawDebugLine(WorldContext->GetWorld(), BottomLeft, TopLeft, Color, false, -1.0f, 1, LineThickness);
#endif
}
}
