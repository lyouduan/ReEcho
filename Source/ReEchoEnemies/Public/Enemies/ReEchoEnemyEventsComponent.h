#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "ReEchoEnemyEventsComponent.generated.h"

USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyActionCommittedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoAttackIdentity Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyArchetype Archetype = EReEchoEnemyArchetype::Grunt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bCanDamageTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bSelfDestructAfterAttack = false;
};

USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyFuseEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float RemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float DurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bStarted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoEnemyActionCommittedDelegate,
                                            const FReEchoEnemyActionCommittedEvent&,
                                            Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoEnemyFuseDelegate, const FReEchoEnemyFuseEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoBossIntentDelegate, const FReEchoBossIntent&, Intent);

/** Presentation-neutral behavior event bus explicitly wired by the enemy host. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHOENEMIES_API UReEchoEnemyEventsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoEnemyActionCommittedDelegate OnActionCommitted;

	UPROPERTY(BlueprintAssignable)
	FReEchoEnemyFuseDelegate OnFuseChanged;

	UPROPERTY(BlueprintAssignable)
	FReEchoBossIntentDelegate OnBossIntent;

	void PublishActionCommitted(const FReEchoEnemyActionCommittedEvent& Event)
	{
		OnActionCommitted.Broadcast(Event);
	}

	void PublishFuseChanged(const FReEchoEnemyFuseEvent& Event)
	{
		OnFuseChanged.Broadcast(Event);
	}

	void PublishBossIntent(const FReEchoBossIntent& Intent)
	{
		OnBossIntent.Broadcast(Intent);
	}
};
