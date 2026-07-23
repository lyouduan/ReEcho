#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoEchoActor.generated.h"

class AReEchoTrajectoryActor;
class AReEchoWeaponActor;
class UBillboardComponent;
class UMaterialInstanceDynamic;
class UReEchoCombatantComponent;
class UReEchoPlaybackComponent;
class UStaticMeshComponent;

UCLASS()

class REECHO_API AReEchoEchoActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoEchoActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializeEcho(const FReEchoRecording& Recording, float Efficiency);
	void AdvanceEcho(float EncounterTime);

private:
	UFUNCTION()
	void HandleReplayedWeapon(FName WeaponId, float RecordedTime);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Shape;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> CharacterSprite;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoPlaybackComponent> Playback;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	UPROPERTY()
	TObjectPtr<AReEchoTrajectoryActor> Trajectory;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EchoMaterial;

	float VisualTime = 0.0f;
	float AutoTargetRange = 1600.0f;
};