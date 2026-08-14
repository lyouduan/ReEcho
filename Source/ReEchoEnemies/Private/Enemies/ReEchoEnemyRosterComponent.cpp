#include "Enemies/ReEchoEnemyRosterComponent.h"

#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "GameFramework/Actor.h"

UReEchoEnemyRosterComponent::UReEchoEnemyRosterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UReEchoEnemyRosterComponent::RegisterEnemy(AActor* Host, UReEchoEnemyLogicComponent* Logic)
{
	if (!IsValid(Host) || !IsValid(Logic))
	{
		return false;
	}

	const FReEchoEnemyLogicSnapshot Candidate = Logic->GetSnapshot();
	for (const FEntry& Entry : Entries)
	{
		if (Entry.Host == Host || (Entry.Logic.IsValid() && Entry.Logic->GetSnapshot().SpawnIndex == Candidate.SpawnIndex))
		{
			return false;
		}
	}

	FEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Host = Host;
	Entry.Logic = Logic;
	return true;
}

void UReEchoEnemyRosterComponent::UnregisterEnemy(const AActor* Host)
{
	Entries.RemoveAll([Host](const FEntry& Entry) { return !Entry.Host.IsValid() || Entry.Host.Get() == Host; });
}

void UReEchoEnemyRosterComponent::ResetRoster()
{
	Entries.Reset();
}

TArray<FReEchoEnemyRosterEntrySnapshot> UReEchoEnemyRosterComponent::GetEntries() const
{
	TArray<FReEchoEnemyRosterEntrySnapshot> Result;
	for (const FEntry& Entry : Entries)
	{
		if (!Entry.Host.IsValid() || !Entry.Logic.IsValid())
		{
			continue;
		}
		const FReEchoEnemyLogicSnapshot LogicSnapshot = Entry.Logic->GetSnapshot();
		FReEchoEnemyRosterEntrySnapshot& Snapshot = Result.AddDefaulted_GetRef();
		Snapshot.Host = Entry.Host;
		Snapshot.Archetype = LogicSnapshot.Archetype;
		Snapshot.SpawnIndex = LogicSnapshot.SpawnIndex;
		Snapshot.bAlive = LogicSnapshot.bAlive;
	}
	Result.Sort([](const FReEchoEnemyRosterEntrySnapshot& Left, const FReEchoEnemyRosterEntrySnapshot& Right) {
		return Left.SpawnIndex < Right.SpawnIndex;
	});
	return Result;
}

TArray<TWeakObjectPtr<AActor>> UReEchoEnemyRosterComponent::GetLivingEnemyActors() const
{
	TArray<TWeakObjectPtr<AActor>> Result;
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : GetEntries())
	{
		if (Entry.bAlive)
		{
			Result.Add(Entry.Host);
		}
	}
	return Result;
}

int32 UReEchoEnemyRosterComponent::GetLivingEnemyCount() const
{
	int32 Count = 0;
	for (const FReEchoEnemyRosterEntrySnapshot& Entry : GetEntries())
	{
		Count += Entry.bAlive ? 1 : 0;
	}
	return Count;
}

bool UReEchoEnemyRosterComponent::HasLivingEnemies() const
{
	return GetLivingEnemyCount() > 0;
}
