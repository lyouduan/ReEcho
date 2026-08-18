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

private:
	bool ExecuteAttack(UReEchoCombatantComponent* Combatant, const FReEchoWeaponAttackCommit& Commit);
	bool RebuildEffectiveDefinition();
	bool ApplyDamageToTarget(AActor& Target,
	                         const FReEchoWeaponAttackCommit& Commit,
	                         const FVector& DamageSource,
	                         UReEchoCombatantComponent* Combatant) const;
	bool FireStaffLightWave(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant);
	bool FireProjectile(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant);
	bool SwingMelee(const FReEchoWeaponAttackCommit& Commit, UReEchoCombatantComponent* Combatant);
	/** Resolve the owner's gameplay aim without requiring the owner root actor to rotate for presentation. */
	FVector ResolveOwnerAimDirection() const;
	const FReEchoCsvWeaponRow* FindEquippedDefinition() const;
	void UpdateElementIndicator();
	void RefreshVisualState();
	void StartSwordAnimation();
	void SpawnSwordArc();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> StaffSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SwordSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> ElementIndicator;

	TMap<FName, FReEchoCsvWeaponRow> Definitions;
	TSharedPtr<const FReEchoCsvDataSnapshot> DataSnapshot;
	FReEchoBuildSnapshot BuildSnapshot;
	FReEchoEffectiveWeaponDefinition EffectiveDefinition;
	bool bHasEffectiveDefinition = false;
	FName EquippedWeaponId;
	FReEchoWeaponLogic WeaponLogic;
	FReEchoWeaponAttackCommit LastAttackCommit;
	FName LastCommittedAttackStepId = NAME_None;
	int32 LastCommittedAttackStepIndex = INDEX_NONE;
	float SwordAnimationTime = 0.0f;
	float SwordAnimationDuration = 0.18f;
	float SwordSwingDirection = -1.0f;
	FVector SwordSpriteRestLocation = FVector(8.0f, 0.0f, 0.0f);
};
