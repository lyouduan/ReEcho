#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoWeaponActor.generated.h"

class AReEchoEnemyActor;
class AReEchoSwordArcActor;
class UReEchoCombatantComponent;
class USceneComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EReEchoWeaponSlot : uint8
{
	None = 0,
	PhysicalOrb = 1,
	Sword = 2,
	ElementalOrb = 3
};

UCLASS()
class REECHO_API AReEchoWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoWeaponActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitializeWeapon();
	void SelectWeapon(EReEchoWeaponSlot NewSlot);
	bool SelectWeaponById(FName WeaponId);
	bool TryBasicAttack(UReEchoCombatantComponent* Combatant);
	bool TryActiveAttack(UReEchoCombatantComponent* Combatant);
	FName GetEquippedWeaponId() const;
	FString GetEquippedWeaponLabel() const;

private:
	struct FWeaponDefinition
	{
		FName WeaponId;
		float Interval = 0.55f;
		float Range = 260.0f;
		float PhysicalCoefficient = 1.0f;
		float ElementalCoefficient = 0.0f;
		float ArcDegrees = 0.0f;
	};

	bool LoadWeaponDefinition(FName WeaponId, FWeaponDefinition& OutDefinition) const;
	bool FireProjectile(
		const FWeaponDefinition& Definition,
		UReEchoCombatantComponent* Combatant,
		const FLinearColor& Color);
	bool SwingSword(
		const FWeaponDefinition& Definition,
		UReEchoCombatantComponent* Combatant);
	void StartSwordAnimation();
	void SpawnSwordArc();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SwordBlade;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SwordGuard;

	TMap<EReEchoWeaponSlot, FWeaponDefinition> Definitions;
	EReEchoWeaponSlot EquippedSlot = EReEchoWeaponSlot::PhysicalOrb;
	float AttackCooldown = 0.0f;
	float SwordAnimationTime = 0.0f;
	float SwordAnimationDuration = 0.34f;
	float SwordSwingDirection = -1.0f;
	FVector SwordRestLocation = FVector::ZeroVector;
};