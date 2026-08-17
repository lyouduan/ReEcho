#pragma once

#include "AbilitySystemInterface.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoEnemyActor.generated.h"

class UAbilitySystemComponent;
class UCapsuleComponent;
class UReEchoCombatAttributeSet;
class UReEchoCombatantComponent;
class UReEchoCombatAudioAdapterComponent;
class UReEchoCombatEventsComponent;
class UReEchoEnemyEventsComponent;
class UReEchoEnemyLogicComponent;
class UReEchoEnemyPresentationComponent;
class UReEchoEnemyRosterComponent;
struct FReEchoEnemyActionIntent;
struct FReEchoEnemyPresentationSnapshot;
struct FReEchoEnemySenseSnapshot;

UENUM(BlueprintType)
enum class EReEchoEnemyKind : uint8
{
	Grunt,
	Shield,
	Bomber,
	Boss,
	Slime,
	Ranged,
	Elite
};

/** Lightweight world host composing Enemy logic, Combat adjudication and read-only presentation. */
UCLASS()

class REECHO_API AReEchoEnemyActor : public AActor, public IAbilitySystemInterface, public IReEchoCombatTarget
{
	GENERATED_BODY()

public:
	AReEchoEnemyActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void Configure(EReEchoEnemyKind InKind, int32 SpawnIndex);
	bool ConfigureFromDefinition(const FReEchoEnemyDefinition& Definition, int32 SpawnIndex);

	void SetEnemyId(FName InEnemyId)
	{
		EnemyId = InEnemyId;
	}

	FName GetEnemyId() const
	{
		return EnemyId;
	}

	void SetEnemyRoster(UReEchoEnemyRosterComponent* InRoster);
	float ReceiveGrayboxDamage(float Damage,
	                           const FVector& SourceLocation,
	                           const FLinearColor& DamageNumberColor = FLinearColor::White,
	                           FReEchoAttackIdentity Attack = {});
	float ReceiveElementalDamage(float Damage,
	                             EReEchoElement Element,
	                             const FVector& SourceLocation,
	                             float ReactionEfficiency = 1.0f,
	                             FReEchoAttackIdentity Attack = {});
	void RefreshElementAttachmentVisual();

	EReEchoElement GetAttachedElement() const;
	const FReEchoElementState& GetElementState() const;
#if WITH_DEV_AUTOMATION_TESTS
	FReEchoElementState& EditElementState();
#endif

	UReEchoCombatantComponent* GetCombatantComponent() const
	{
		return Combatant;
	}

	UReEchoEnemyLogicComponent* GetEnemyLogicComponent() const
	{
		return EnemyLogic;
	}

	UReEchoEnemyEventsComponent* GetEnemyEventsComponent() const
	{
		return EnemyEvents;
	}

	bool IsAlive() const;

	virtual bool IsCombatTargetAlive() const override
	{
		return IsAlive();
	}

	virtual FVector GetCombatTargetLocation() const override
	{
		return GetActorLocation();
	}

	virtual int32 GetCombatTargetTieBreakIndex() const override
	{
		return GetSpawnIndex();
	}

	virtual UReEchoCombatantComponent* GetCombatTargetCombatant() const override
	{
		return Combatant;
	}

	virtual float ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const override;

	virtual bool
	IntersectsCombatPath(const FVector& PathStart, const FVector& PathEnd, float CarrierRadius) const override
	{
		return IntersectsProjectilePath(PathStart, PathEnd, CarrierRadius);
	}

	int32 GetSpawnIndex() const;
	EReEchoEnemyKind GetKind() const;
	bool IntersectsProjectilePath(const FVector& PathStart, const FVector& PathEnd, float ProjectileRadius) const;
	FReEchoEnemyRuntimeState CaptureRuntimeState() const;
	void RestoreRuntimeState(const FReEchoEnemyRuntimeState& SavedState);
#if WITH_DEV_AUTOMATION_TESTS
	FReEchoEnemyActionIntent AdvanceBehaviorForTests(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FName EnemyId = NAME_None;
	void BindComposedComponents();
	FReEchoEnemyActionIntent AdvanceBehavior(const FReEchoEnemySenseSnapshot& Sense, float DeltaSeconds);
	void ApplyActionIntent(const FReEchoEnemyActionIntent& Intent);
	void ApplyBossIntent(const struct FReEchoBossIntent& Intent);
	void ApplyBossHit(const struct FReEchoBossIntent& Intent, AActor* Target, const FVector& HitLocation);
	void AdvanceBossProjectiles(float DeltaSeconds);
	FVector ResolveBossTeleportDestination(const FVector& TargetLocation);
	FReEchoEnemyPresentationSnapshot BuildPresentationSnapshot(bool bMoving) const;

	UFUNCTION()
	void HandleCombatDeath(const FReEchoDamageEvent& Event);

	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UReEchoCombatAttributeSet> CombatAttributes;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Collision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatAudioAdapterComponent> CombatAudioAdapter;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyLogicComponent> EnemyLogic;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyEventsComponent> EnemyEvents;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyPresentationComponent> EnemyPresentation;
	UPROPERTY()
	TObjectPtr<UReEchoEnemyRosterComponent> EnemyRoster;

	UPROPERTY()
	TArray<FReEchoEnemyProjectileRuntimeState> BossProjectiles;

	bool bVisualPlacementApplied = false;
	bool bAudioSpawnPosted = false;
};
