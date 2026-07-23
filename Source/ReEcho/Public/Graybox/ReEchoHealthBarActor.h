#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoHealthBarActor.generated.h"

class UReEchoCombatantComponent;
class USceneComponent;
class UWidgetComponent;

UCLASS()
class REECHO_API AReEchoHealthBarActor : public AActor
{
	GENERATED_BODY()
public:
	AReEchoHealthBarActor();
	virtual void Tick(float DeltaSeconds) override;
	void Initialize(UReEchoCombatantComponent* InCombatant,
	                const FLinearColor& FillColor,
	                float InHeight,
	                float InWidthScale = 1.f);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;
	UPROPERTY()
	TObjectPtr<UWidgetComponent> Widget;
	TWeakObjectPtr<UReEchoCombatantComponent> Combatant;
	float Height = 100.f;
};

