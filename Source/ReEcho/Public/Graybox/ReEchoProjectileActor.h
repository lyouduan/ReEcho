#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Weapons/ReEchoProjectileLogicComponent.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "GameFramework/Actor.h"
#include "ReEchoProjectileActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UNiagaraComponent;
class AReEchoWeaponActor;

UCLASS()

class REECHO_API AReEchoProjectileActor : public AActor
{
	GENERATED_BODY()
public:
	AReEchoProjectileActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeProjectile(const FVector& Direction,
	                          float InDamage,
	                          const FVector& InDamageSource,
	                          const FLinearColor& Color,
	                          EReEchoElement InElement = EReEchoElement::None,
	                          float InReactionEfficiency = 1.0f,
	                          float InExplosionRadiusCm = 0.0f,
	                          float InMaxRangeCm = 0.0f,
	                          FReEchoAttackIdentity InAttack = {},
	                          bool bInCritical = false,
	                          EReEchoDamageSource InDamageSourceType = EReEchoDamageSource::Player,
	                          FName InWeaponVisualKey = NAME_None,
	                          bool bInPierceOnCritical = false,
	                          AReEchoWeaponActor* InRuneHost = nullptr,
	                          TSharedPtr<FReEchoWeaponRuneAttackContext> InRuneContext = nullptr,
	                          bool bInAllowSplit = true);

	/** Weapon-specific presentation asset contract. Empty means procedural fallback. */
	static FString ResolveWeaponTexturePath(FName WeaponVisualKey);

	float GetDamage() const
	{
		return Damage;
	}

	EReEchoElement GetElement() const
	{
		return Element;
	}

	float GetExplosionRadiusCm() const
	{
		return ExplosionRadiusCm;
	}

	FVector GetVelocity() const
	{
		return ProjectileLogic ? ProjectileLogic->GetSnapshot().Velocity : FVector::ZeroVector;
	}

private:
	void HandleProjectileImpact(const FReEchoProjectileSnapshot& Snapshot, const FReEchoHitResolved& Result);
	/** Returns true only when the dedicated element-specific Gun flight Niagara was spawned. */
	bool ConfigureWeaponNiagara(FName WeaponVisualKey, const FVector& Direction);
	void SpawnWeaponImpactNiagara(const FVector& Location, const FVector& Direction);

	UPROPERTY()
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shape;
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> ElementLabel;
	UPROPERTY()
	TObjectPtr<UReEchoProjectileLogicComponent> ProjectileLogic;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> FlightEffect;
	float Damage = 1.f;
	EReEchoElement Element = EReEchoElement::None;
	float Speed = 950.f;
	float ExplosionRadiusCm = 0.0f;
	FName WeaponVisualKey;
	bool bImpactVfxSpawned = false;
	TWeakObjectPtr<AReEchoWeaponActor> RuneHost;
	TSharedPtr<FReEchoWeaponRuneAttackContext> RuneContext;
	bool bAllowSplit = true;
	void ConfigureWeaponVisual(FName InWeaponVisualKey, const FLinearColor& Color);
};
