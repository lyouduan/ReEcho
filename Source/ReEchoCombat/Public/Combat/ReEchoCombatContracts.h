#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "Components/ActorComponent.h"
#include "ReEchoCombatContracts.generated.h"

class UReEchoCombatantComponent;

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoAttackCommittedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Target = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RawDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AppliedDamage = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement Element = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCritical = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bBlocked = false;
	/** This damage reduced an alive target to zero health. Consumers must suppress ordinary Hurt presentation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bFatal = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector WorldLocation = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector SourceWorldLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoHealthChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UReEchoCombatantComponent> Combatant = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PreviousHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CurrentHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Reason = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoDamageSource DamageSource = EReEchoDamageSource::Player;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoCombatantSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UReEchoCombatantComponent> Combatant = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CurrentHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaximumHealth = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoStatBlock Stats;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoElementState ElementState;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bAlive = false;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoElementStateChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UReEchoCombatantComponent> Combatant = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoElementState State;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoElementReactionLink
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> SourceTarget = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> TargetTarget = nullptr;
};

USTRUCT(BlueprintType)

struct REECHOCOMBAT_API FReEchoElementReactionResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ReactionId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ReactionBehaviorId = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RadiusCm = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement PreviousElement = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement IncomingElement = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EReEchoElement ResultingElement = EReEchoElement::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> PrimaryTarget = nullptr;
	/** Authoritative stable gameplay order; Growth contains only newly attached targets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<TObjectPtr<AActor>> AffectedTargets;
	/** Authoritative Conduct discovery edges; empty for non-link reactions. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<FReEchoElementReactionLink> ReactionLinks;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoElementStateChangedDelegate,
                                            const FReEchoElementStateChangedEvent&,
                                            Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoElementReactionResolvedDelegate,
                                            const FReEchoElementReactionResolvedEvent&,
                                            Event);

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
	UPROPERTY(BlueprintAssignable) FReEchoElementStateChangedDelegate OnElementStateChanged;
	UPROPERTY(BlueprintAssignable) FReEchoElementReactionResolvedDelegate OnElementReactionResolved;

	void PublishAttackCommitted(const FReEchoAttackCommittedEvent& Event)
	{
#if WITH_DEV_AUTOMATION_TESTS
		++AttackCommittedPublishCountForTests;
		LastAttackCommittedEventForTests = Event;
#endif
		OnAttackCommitted.Broadcast(Event);
	}

	void PublishHit(const FReEchoDamageEvent& Event)
	{
		OnHit.Broadcast(Event);
	}

	void PublishHurt(const FReEchoDamageEvent& Event)
	{
#if WITH_DEV_AUTOMATION_TESTS
		++HurtPublishCountForTests;
		LastHurtEventForTests = Event;
#endif
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

	void PublishElementStateChanged(const FReEchoElementStateChangedEvent& Event)
	{
#if WITH_DEV_AUTOMATION_TESTS
		++ElementStatePublishCountForTests;
#endif
		OnElementStateChanged.Broadcast(Event);
	}

	void PublishElementReactionResolved(const FReEchoElementReactionResolvedEvent& Event)
	{
#if WITH_DEV_AUTOMATION_TESTS
		++ElementReactionPublishCountForTests;
		LastElementReactionEventForTests = Event;
#endif
		OnElementReactionResolved.Broadcast(Event);
	}

#if WITH_DEV_AUTOMATION_TESTS
	int32 GetAttackCommittedPublishCountForTests() const
	{
		return AttackCommittedPublishCountForTests;
	}

	const FReEchoAttackCommittedEvent& GetLastAttackCommittedEventForTests() const
	{
		return LastAttackCommittedEventForTests;
	}

	int32 GetElementStatePublishCountForTests() const
	{
		return ElementStatePublishCountForTests;
	}

	int32 GetElementReactionPublishCountForTests() const
	{
		return ElementReactionPublishCountForTests;
	}

	int32 GetHurtPublishCountForTests() const
	{
		return HurtPublishCountForTests;
	}

	const FReEchoDamageEvent& GetLastHurtEventForTests() const
	{
		return LastHurtEventForTests;
	}

	const FReEchoElementReactionResolvedEvent& GetLastElementReactionEventForTests() const
	{
		return LastElementReactionEventForTests;
	}

private:
	int32 AttackCommittedPublishCountForTests = 0;
	int32 ElementStatePublishCountForTests = 0;
	int32 ElementReactionPublishCountForTests = 0;
	int32 HurtPublishCountForTests = 0;
	FReEchoDamageEvent LastHurtEventForTests;
	FReEchoAttackCommittedEvent LastAttackCommittedEventForTests;
	FReEchoElementReactionResolvedEvent LastElementReactionEventForTests;
#endif
};
