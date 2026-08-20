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

UENUM(BlueprintType)
enum class EReEchoEnemySpecialActionEventType : uint8
{
	WindupStarted,
	ActionCommitted,
	ActionEnded
};

/** Presentation-neutral transition emitted when a non-Boss special action changes phase. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoEnemySpecialActionEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemySpecialActionEventType Type = EReEchoEnemySpecialActionEventType::WindupStarted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoAttackIdentity Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector LockedDirection = FVector::ForwardVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector LockedTargetLocation = FVector::ZeroVector;
};

UENUM(BlueprintType)
enum class EReEchoEnemyProjectileEventType : uint8
{
	Spawned,
	Moved,
	Ended
};

/** Read-only projectile lifecycle for presentation. Motion and collision remain Enemy Host authority. */
USTRUCT(BlueprintType)

struct REECHOENEMIES_API FReEchoEnemyProjectileEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyProjectileEventType Type = EReEchoEnemyProjectileEventType::Spawned;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoAttackIdentity Attack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName AbilityId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Direction = FVector::ForwardVector;

	/** Stable identity inside one committed volley. INDEX_NONE is reserved for legacy single projectiles. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 VolleyBallIndex = INDEX_NONE;

	/** Read-only size context for the visual proxy; gameplay collision remains Enemy Host authority. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CollisionRadiusCm = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoEnemyActionCommittedDelegate,
                                            const FReEchoEnemyActionCommittedEvent&,
                                            Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoEnemyFuseDelegate, const FReEchoEnemyFuseEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoBossIntentDelegate, const FReEchoBossIntent&, Intent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoEnemySpecialActionDelegate,
                                            const FReEchoEnemySpecialActionEvent&,
                                            Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoEnemyProjectileDelegate, const FReEchoEnemyProjectileEvent&, Event);

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

	UPROPERTY(BlueprintAssignable)
	FReEchoEnemySpecialActionDelegate OnSpecialAction;

	UPROPERTY(BlueprintAssignable)
	FReEchoEnemyProjectileDelegate OnProjectile;

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

	void PublishSpecialAction(const FReEchoEnemySpecialActionEvent& Event)
	{
		OnSpecialAction.Broadcast(Event);
	}

	void PublishProjectile(const FReEchoEnemyProjectileEvent& Event)
	{
#if WITH_DEV_AUTOMATION_TESTS
		PublishedProjectileEventsForTests.Add(Event);
#endif
		OnProjectile.Broadcast(Event);
	}

#if WITH_DEV_AUTOMATION_TESTS
	const TArray<FReEchoEnemyProjectileEvent>& GetPublishedProjectileEventsForTests() const
	{
		return PublishedProjectileEventsForTests;
	}

	void ClearPublishedProjectileEventsForTests()
	{
		PublishedProjectileEventsForTests.Reset();
	}

private:
	TArray<FReEchoEnemyProjectileEvent> PublishedProjectileEventsForTests;
#endif
};
