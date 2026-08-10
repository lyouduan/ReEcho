#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoWeaponActor.generated.h"

class AReEchoEnemyActor;
class UReEchoCombatantComponent;
class UBillboardComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/** 玩家武器控制器：配置本局锁定武器，并管理冷却及远近程攻击表现。 */
UCLASS()

class REECHO_API AReEchoWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoWeaponActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeWeapon();
	bool ConfigureLockedWeapon(FName WeaponId);
	/** 在冷却允许时执行当前武器基础攻击，并返回是否成功出手。 */
	bool TryBasicAttack(UReEchoCombatantComponent* Combatant);
	bool TryActiveAttack(UReEchoCombatantComponent* Combatant);
	/** Executes an attack without the legacy weapon timer; player GAS owns cooldown. */
	bool ExecuteBasicAttack(UReEchoCombatantComponent* Combatant);
	float GetAttackInterval(UReEchoCombatantComponent* Combatant) const;
	FName GetEquippedWeaponId() const;
	FString GetEquippedWeaponLabel() const;

private:
	void ApplyWeaponSlot(EReEchoWeaponSlot NewSlot);
	bool ExecuteAttack(UReEchoCombatantComponent* Combatant);
	bool FireStaffLightWave(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant);
	bool FireProjectile(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant);
	bool SwingSword(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant);
	EReEchoElement ConsumeNextElement();
	EReEchoElement PeekNextElement() const;
	float ApplyRoleDamageModifiers(float BaseDamage, const FReEchoStatBlock& Stats);
	void UpdateElementIndicator();
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

	TMap<EReEchoWeaponSlot, FReEchoWeaponConfig> Definitions;
	EReEchoWeaponSlot EquippedSlot = EReEchoWeaponSlot::PhysicalOrb;
	float AttackCooldown = 0.0f;
	float SwordAnimationTime = 0.0f;
	float SwordAnimationDuration = 0.18f;
	float SwordSwingDirection = -1.0f;
	int32 NextElementIndex = 0;
	int32 AttackSequence = 0;
	float CriticalAccumulator = 0.0f;
	FVector SwordSpriteRestLocation = FVector(8.0f, 0.0f, 0.0f);
};
