#pragma once

#include "Components/ActorComponent.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "ReEchoEnemyRosterComponent.generated.h"

class UReEchoEnemyLogicComponent;

/** Stable, read-only roster view. The weak host handle never owns the world Actor. */
USTRUCT(BlueprintType)
struct REECHOENEMIES_API FReEchoEnemyRosterEntrySnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<AActor> Host;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoEnemyArchetype Archetype = EReEchoEnemyArchetype::Grunt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 SpawnIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bAlive = false;
};

/** Encounter-owned registry replacing repeated world scans for enemy lifetime and save capture. */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHOENEMIES_API UReEchoEnemyRosterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoEnemyRosterComponent();

	bool RegisterEnemy(AActor* Host, UReEchoEnemyLogicComponent* Logic);
	void UnregisterEnemy(const AActor* Host);
	void ResetRoster();

	TArray<FReEchoEnemyRosterEntrySnapshot> GetEntries() const;
	TArray<TWeakObjectPtr<AActor>> GetLivingEnemyActors() const;
	int32 GetLivingEnemyCount() const;
	bool HasLivingEnemies() const;

private:
	struct FEntry
	{
		TWeakObjectPtr<AActor> Host;
		TWeakObjectPtr<UReEchoEnemyLogicComponent> Logic;
	};

	TArray<FEntry> Entries;
};
