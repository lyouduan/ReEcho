#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "Components/ActorComponent.h"
#include "ReEchoCombatContracts.generated.h"

class UReEchoCombatantComponent;

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoAttackCommitId
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int64 Value = 0;

	bool IsValid() const
	{
		return Value != 0;
	}
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoAttackCommittedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackCommitId CommitId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Source = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName WeaponId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackPatternId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackStepId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 StepIndex = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Origin = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Direction = FVector::ForwardVector;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoDamageEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackCommitId CommitId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Source = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RawDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AppliedDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement Element = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCritical = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bBlocked = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector WorldLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoHealthChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UReEchoCombatantComponent> Combatant = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PreviousHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CurrentHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumHealth = 0.0f;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoCombatantSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UReEchoCombatantComponent> Combatant = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CurrentHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoStatBlock Stats;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bAlive = false;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoAttackSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoAttackMode Mode = EReEchoAttackMode::Automatic;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bAttackHeld = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> CurrentTarget = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName WeaponId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AttackStepId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NextAttackRemaining = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AttackSpeed = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoAttackCommittedDelegate, const FReEchoAttackCommittedEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoDamageDelegate, const FReEchoDamageEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoHealthChangedEventDelegate, const FReEchoHealthChangedEvent&, Event);

UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))

class REECHOCOMBAT_API UReEchoCombatEventsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FReEchoAttackCommittedDelegate OnAttackCommitted;
	UPROPERTY(BlueprintAssignable) FReEchoDamageDelegate OnHit;
	UPROPERTY(BlueprintAssignable) FReEchoDamageDelegate OnHurt;
	UPROPERTY(BlueprintAssignable) FReEchoDamageDelegate OnKill;
	UPROPERTY(BlueprintAssignable) FReEchoDamageDelegate OnDeath;
	UPROPERTY(BlueprintAssignable) FReEchoHealthChangedEventDelegate OnHealthChanged;

	void PublishAttackCommitted(const FReEchoAttackCommittedEvent& Event)
	{
		OnAttackCommitted.Broadcast(Event);
	}

	void PublishHit(const FReEchoDamageEvent& Event)
	{
		OnHit.Broadcast(Event);
	}

	void PublishHurt(const FReEchoDamageEvent& Event)
	{
		OnHurt.Broadcast(Event);
	}

	void PublishKill(const FReEchoDamageEvent& Event)
	{
		OnKill.Broadcast(Event);
	}

	void PublishDeath(const FReEchoDamageEvent& Event)
	{
		OnDeath.Broadcast(Event);
	}

	void PublishHealthChanged(const FReEchoHealthChangedEvent& Event)
	{
		OnHealthChanged.Broadcast(Event);
	}
};

/** Single source of truth for basic-attack readiness. */
struct REECHOCOMBAT_API FReEchoWeaponCadence
{
	bool TryCommit(float IntervalSeconds, FReEchoAttackCommitId& OutCommitId);
	void Tick(float DeltaSeconds);
	void Reset();

	void CancelReadiness()
	{
		RemainingSeconds = 0.0f;
	}

	float GetRemaining() const
	{
		return RemainingSeconds;
	}

	bool IsReady() const
	{
		return RemainingSeconds <= 0.0f;
	}

private:
	float RemainingSeconds = 0.0f;
	int64 LastCommitId = 0;
};
