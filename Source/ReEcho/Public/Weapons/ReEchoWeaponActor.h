#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "Weapons/ReEchoWeaponLogic.h"
#include "GameFramework/Actor.h"
#include "ReEchoWeaponActor.generated.h"

class UReEchoCombatantComponent;
class UBillboardComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** 武器执行器：按构筑快照完成初始化，并管理冷却及远近程攻击表现。 */
UCLASS()

class REECHO_API AReEchoWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoWeaponActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeWeapon(const FReEchoBuildSnapshot* InBuildSnapshot = nullptr,
	                      TSharedPtr<const FReEchoCsvDataSnapshot> InSnapshot = nullptr);
	bool SelectWeaponById(FName WeaponId);
	/** Equip a rune (weapon part) into the first compatible, capacity-available slot. Returns false (state unchanged)
	 * if invalid / incompatible / over-capacity / duplicate. Rebuilds the effective weapon definition so the weapon
	 * "is" its runes (Plan75). */
	bool EquipRune(FName PartId, FString& OutError);
	/** Remove all runes in the given slot type and rebuild. Returns false if validation fails. */
	bool UnequipRune(FName SlotTypeId, FString& OutError);

	/** In-run live rune loadout (mirrors BuildSnapshot.EquippedParts after every Equip/Unequip). */
	const TArray<FReEchoEquippedPartSnapshot>& GetEquippedRunes() const
	{
		return EquippedRunes;
	}

	/** 在冷却允许时执行当前武器基础攻击，并返回是否成功出手。 */
	bool TryBasicAttack(UReEchoCombatantComponent* Combatant);
	bool TryActiveAttack(UReEchoCombatantComponent* Combatant);
	/** Compatibility alias; the weapon cadence remains the sole basic-attack gate. */
	bool ExecuteBasicAttack(UReEchoCombatantComponent* Combatant);
	float GetAttackInterval(UReEchoCombatantComponent* Combatant) const;
	/** 当前武器当前攻击步骤的有效攻击距离（厘米）。供自动攻击在射程内选择目标，不复制 Echo 固定 AutoTargetRange。 */
	float GetCurrentAttackRangeCm() const;

	/** 武器当前动作锁（有序攻击步骤锁）剩余秒数，供 held 普攻循环判断是否临时忙。 */
	float GetActionLockRemaining() const
	{
		return WeaponLogic.GetSnapshot().ReadinessRemainingSeconds;
	}

	/** 已成功执行的普攻次数（命中或非命中均计数），供确定性回归测试观察。 */
	int32 GetAttackSequence() const
	{
		return WeaponLogic.GetSnapshot().SuccessfulAttackCount;
	}

	float GetAttackCooldownRemaining() const;
#if WITH_DEV_AUTOMATION_TESTS
	float GetStepLockRemaining() const
	{
		return WeaponLogic.GetSnapshot().BehaviorRemainingSeconds;
	}

	TSharedPtr<FReEchoWeaponRuneAttackContext>
	BuildRuneAttackContextForTests(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant) const;
	void ProcessRuneHitForTests(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
	                            const FReEchoHitResolved& Result);
	void ProcessRuneAttackForTests(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	void ProcessRuneOnAttackForTests(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	FReEchoHitResolved ApplyRuneDamageForTests(AActor& Target,
	                                           const FReEchoWeaponAttackCommit& Commit,
	                                           const FVector& DamageSource,
	                                           UReEchoCombatantComponent* Combatant,
	                                           const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	float GetTimedRangeMultiplierForTests() const;
	bool IsScytheThrownForTests() const;
	void AdvanceScytheThrowForTests(float DeltaSeconds);
#endif
	FName GetCurrentAttackStepId() const;

	FName GetLastCommittedAttackStepId() const
	{
		return LastCommittedAttackStepId;
	}

	int32 GetLastCommittedAttackStepIndex() const
	{
		return LastCommittedAttackStepIndex;
	}

	FName GetAttackPatternId() const;

	FReEchoAttackIdentity GetLastAttackIdentity() const
	{
		return LastAttackCommit.Attack;
	}

	FName GetEquippedWeaponId() const;
	/** Stable data-authored presentation identity; never infer visuals from WeaponId. */
	FName GetEquippedWeaponVisualKey() const;
	FString GetEquippedWeaponLabel() const;
	const FReEchoBuildSnapshot& GetBuildSnapshot() const;
	FString GetPinnedWeaponDomainRevision() const;
	bool IsInvulnerableWindowActive() const;
	void HandleProjectileResolved(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
	                              const FReEchoProjectileSnapshot& Snapshot,
	                              const FReEchoHitResolved& Result,
	                              bool bAllowSplit);

private:
	bool ExecuteAttack(UReEchoCombatantComponent* Combatant, const FReEchoWeaponAttackCommit& Commit);
	bool RebuildEffectiveDefinition();
	FReEchoHitResolved ApplyDamageToTarget(AActor& Target,
	                                       const FReEchoWeaponAttackCommit& Commit,
	                                       const FVector& DamageSource,
	                                       UReEchoCombatantComponent* Combatant,
	                                       const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
	                                       float DamageMultiplier = 1.0f,
	                                       bool bProcessRuneEffects = true);
	bool FireStaffLightWave(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant);
	bool FireProjectile(const FReEchoWeaponAttackCommit& Commit,
	                    UReEchoCombatantComponent* Combatant,
	                    const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	bool SwingMelee(const FReEchoWeaponAttackCommit& Commit,
	                UReEchoCombatantComponent* Combatant,
	                const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	TSharedPtr<FReEchoWeaponRuneAttackContext> BuildRuneAttackContext(const FReEchoWeaponAttackCommit& Commit,
	                                                                  UReEchoCombatantComponent* Combatant) const;
	void ProcessResolvedHit(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
	                        const FReEchoHitResolved& Result);
	void ProcessAttackResolved(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	void ProcessOnAttackRuneEffects(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	void SpawnSplitProjectiles(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context,
	                           const FReEchoProjectileSnapshot& Snapshot,
	                           const FReEchoHitResolved& Result,
	                           const FReEchoWeaponRuneEffectSpec& Effect);
	void SpawnTimeShardPickup(const FVector& Location, int32 Amount) const;
	bool HasRuneBehavior(FName BehaviorId) const;
	float GetRuneParam(FName BehaviorId, FName ParamName, float DefaultValue) const;
	float GetRuneEffectValue(FName BehaviorId, FName ParamName, float DefaultValue) const;
	float GetTimedRangeMultiplier() const;
	void BeginScytheThrow(const TSharedPtr<FReEchoWeaponRuneAttackContext>& Context);
	void RecallScythe();
	void AdvanceScytheThrow(float DeltaSeconds);
	/** Resolve the owner's gameplay aim without requiring the owner root actor to rotate for presentation. */
	FVector ResolveOwnerAimDirection() const;
	EReEchoDamageSource ResolveOwnerDamageSource() const;
	const FReEchoCsvWeaponRow* FindEquippedDefinition() const;
	void UpdateElementIndicator();
	void RefreshVisualState();
	void StartMeleeAnimation(FName WeaponVisualKey);
	void SpawnMeleeArc(FName WeaponVisualKey);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> StaffSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SwordSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> ElementIndicator;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> ScytheSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> WhipSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> BowSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> GunSprite;

	TMap<FName, FReEchoCsvWeaponRow> Definitions;
	TSharedPtr<const FReEchoCsvDataSnapshot> DataSnapshot;
	FReEchoBuildSnapshot BuildSnapshot;
	/** Plan75: in-run weapon rune loadout (3 slot types per weapon, capacity from SlotProfiles). Mirror of
	 * BuildSnapshot.EquippedParts; the live authority that Equip/Unequip mutate, then drive BuildSnapshot + effective
	 * definition rebuild. Transient (not UPROPERTY-serialized); BuildSnapshot.EquippedParts is the save/replay truth.
	 */
	TArray<FReEchoEquippedPartSnapshot> EquippedRunes;
	FReEchoEffectiveWeaponDefinition EffectiveDefinition;
	bool bHasEffectiveDefinition = false;
	FName EquippedWeaponId;
	FReEchoWeaponLogic WeaponLogic;
	FReEchoWeaponAttackCommit LastAttackCommit;
	TSharedPtr<FReEchoWeaponRuneAttackContext> LastRuneAttackContext;
	TMap<FName, int32> PersistentRuneHitCounters;
	TMap<FName, TMap<TWeakObjectPtr<AActor>, int32>> PersistentTargetHitCounters;

	struct FTimedRangeStack
	{
		FName SourceId = NAME_None;
		float BonusFraction = 0.0f;
		float ExpiresAt = 0.0f;
	};

	TArray<FTimedRangeStack> TimedRangeStacks;
	float LastRuneAttackWorldTime = -1.0f;
	float NextComboDecayWorldTime = -1.0f;
	bool bScytheThrown = false;
	bool bScytheStationary = false;
	FVector ScytheThrowOrigin = FVector::ZeroVector;
	FVector ScytheThrowLocation = FVector::ZeroVector;
	FVector ScytheThrowDirection = FVector::ForwardVector;
	float ScytheTravelledCm = 0.0f;
	float ScytheTickAccumulator = 0.0f;
	TSet<TWeakObjectPtr<AActor>> ScytheInitialHitTargets;
	TSharedPtr<FReEchoWeaponRuneAttackContext> ScytheRuneContext;
	FName LastCommittedAttackStepId = NAME_None;
	int32 LastCommittedAttackStepIndex = INDEX_NONE;
	float SwordAnimationTime = 0.0f;
	float SwordAnimationDuration = 0.18f;
	float SwordSwingDirection = -1.0f;
	FVector SwordSpriteRestLocation = FVector(8.0f, 0.0f, 0.0f);
};
