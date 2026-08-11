#pragma once

#include "CoreMinimal.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "GameFramework/Actor.h"
#include "ReEchoWeaponActor.generated.h"

class AReEchoEnemyActor;
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
	/** Executes an attack without the legacy weapon timer; player GAS owns cooldown. */
	bool ExecuteBasicAttack(UReEchoCombatantComponent* Combatant);
	float GetAttackInterval(UReEchoCombatantComponent* Combatant) const;
	float GetAttackCooldownRemaining() const;
	FName GetEquippedWeaponId() const;
	FString GetEquippedWeaponLabel() const;
	const FReEchoBuildSnapshot& GetBuildSnapshot() const;
	FString GetPinnedWeaponDomainRevision() const;
	bool IsInvulnerableWindowActive() const;

private:
	bool ExecuteAttack(UReEchoCombatantComponent* Combatant);
	bool RebuildEffectiveDefinition();
	const FReEchoCsvAttackStepRow* ResolveNextAttackStep() const;
	void BeginAttackStep(const FReEchoCsvAttackStepRow& Step);
	float ComputeStepDamage(const FReEchoCsvAttackStepRow& Step, const FReEchoStatBlock& Stats, bool bElemental);
	bool ApplyDamageToEnemy(AReEchoEnemyActor& Enemy,
	                        float Damage,
	                        const FVector& DamageSource,
	                        AActor* DamageCauser,
	                        UReEchoCombatantComponent* Combatant,
	                        EReEchoElement Element) const;
	bool FireStaffLightWave(const FReEchoEffectiveWeaponDefinition& Definition,
	                        const FReEchoCsvAttackStepRow& Step,
	                        UReEchoCombatantComponent* Combatant);
	bool FireProjectile(const FReEchoEffectiveWeaponDefinition& Definition,
	                    const FReEchoCsvAttackStepRow& Step,
	                    UReEchoCombatantComponent* Combatant);
	bool SwingMelee(const FReEchoEffectiveWeaponDefinition& Definition,
	                const FReEchoCsvAttackStepRow& Step,
	                UReEchoCombatantComponent* Combatant);
	const FReEchoCsvWeaponRow* FindEquippedDefinition() const;
	EReEchoElement ConsumeNextElement();
	EReEchoElement PeekNextElement() const;
	float ApplyRoleDamageModifiers(float BaseDamage, const FReEchoStatBlock& Stats);
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
	float AttackCooldown = 0.0f;
	float StepLockRemaining = 0.0f;
	float InvulnerableRemaining = 0.0f;
	int32 NextStepCursor = 0;
	float SwordAnimationTime = 0.0f;
	float SwordAnimationDuration = 0.18f;
	float SwordSwingDirection = -1.0f;
	int32 NextElementIndex = 0;
	int32 AttackSequence = 0;
	float CriticalAccumulator = 0.0f;
	FVector SwordSpriteRestLocation = FVector(8.0f, 0.0f, 0.0f);
};
