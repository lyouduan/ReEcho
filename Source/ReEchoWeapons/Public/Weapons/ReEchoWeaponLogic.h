#pragma once

#include "Weapons/ReEchoWeaponTypes.h"

/** Authoritative, presentation-free weapon state. */
class REECHOWEAPONS_API FReEchoWeaponLogic
{
public:
	bool Initialize(const FReEchoWeaponDefinition& InDefinition);
	void Tick(float DeltaSeconds);
	void Reset();

	bool TryCommitBasicAttack(AActor* Source, const FReEchoStatBlock& Stats, FReEchoWeaponAttackCommit& OutCommit);
	bool TryCommitActiveAttack(AActor* Source, const FReEchoStatBlock& Stats, FReEchoWeaponAttackCommit& OutCommit);
	void ConfirmLastCommit();
	void RollbackLastCommit();

	FReEchoWeaponSnapshot GetSnapshot() const;
	const FReEchoWeaponDefinition& GetDefinition() const;
	const FReEchoWeaponStepDefinition* GetNextStep() const;
	float GetAttackInterval(const FReEchoStatBlock& Stats) const;
	float GetScaledAttackDuration(float InitialDurationSeconds, const FReEchoStatBlock& Stats) const;
	float GetCurrentRangeCm() const;
	float GetOnKillHealPercent() const;
	EReEchoElement PeekNextElement() const;

private:
	bool TryCommit(AActor* Source,
	               const FReEchoStatBlock& Stats,
	               bool bRequireReadiness,
	               FReEchoWeaponAttackCommit& OutCommit);
	EReEchoElement ResolveElement();
	float ComputeDamage(const FReEchoWeaponStepDefinition& Step,
	                    const FReEchoStatBlock& Stats,
	                    EReEchoElement Element,
	                    bool& bOutCritical);

	FReEchoWeaponDefinition Definition;
	float ReadinessRemainingSeconds = 0.0f;
	float BehaviorRemainingSeconds = 0.0f;
	float InvulnerableRemainingSeconds = 0.0f;
	float CriticalAccumulator = 0.0f;
	int64 LastAttackSequence = 0;
	int32 NextStepCursor = 0;
	int32 SuccessfulAttackCount = 0;
	int32 NextElementIndex = 0;
	bool bInitialized = false;

	struct FCommitCheckpoint
	{
		float ReadinessRemainingSeconds = 0.0f;
		float BehaviorRemainingSeconds = 0.0f;
		float InvulnerableRemainingSeconds = 0.0f;
		float CriticalAccumulator = 0.0f;
		int32 NextStepCursor = 0;
		int32 SuccessfulAttackCount = 0;
		int32 NextElementIndex = 0;
	};

	FCommitCheckpoint CommitCheckpoint;
	bool bHasUnconfirmedCommit = false;
};
