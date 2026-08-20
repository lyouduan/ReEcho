#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoSwordArcActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** 玩家挥砍时短暂出现的2D月牙轨迹。 */
UCLASS()
class REECHO_API AReEchoSwordArcActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoSwordArcActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeArc(float SwingDirection, FName WeaponVisualKey);
	static FString ResolveWeaponTexturePath(FName WeaponVisualKey);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SlashSprite;

	FVector SlashBaseScale = FVector::OneVector;
	float ElapsedTime = 0.0f;
	float Lifetime = 0.26f;
	void ConfigureWeaponVisual(FName WeaponVisualKey);
};
