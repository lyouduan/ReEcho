#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoBalanceSettings.h"
#include "GameFramework/Actor.h"
#include "ReEchoWeaponActor.generated.h"

class AReEchoEnemyActor;
class UReEchoCombatantComponent;
class UBillboardComponent;
class USceneComponent;
class UStaticMeshComponent;

/** 玩家武器控制器：管理武器切换、冷却及远近程攻击表现。 */
UCLASS()
class REECHO_API AReEchoWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoWeaponActor();
	virtual void Tick(float DeltaSeconds) override;
	void InitializeWeapon();
	void SelectWeapon(EReEchoWeaponSlot NewSlot);
	bool SelectWeaponById(FName WeaponId);
	/** 在冷却允许时执行当前武器基础攻击，并返回是否成功出手。 */
	bool TryBasicAttack(UReEchoCombatantComponent* Combatant);
	bool TryActiveAttack(UReEchoCombatantComponent* Combatant);
	FName GetEquippedWeaponId() const;
	FString GetEquippedWeaponLabel() const;

private:
	bool FireStaffLightWave(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant);
	bool FireProjectile(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant);
	bool SwingSword(const FReEchoWeaponConfig& Definition, UReEchoCombatantComponent* Combatant);
	void StartSwordAnimation();
	void SpawnSwordArc();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> StaffSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> SwordSprite;

	TMap<EReEchoWeaponSlot, FReEchoWeaponConfig> Definitions;
	EReEchoWeaponSlot EquippedSlot = EReEchoWeaponSlot::PhysicalOrb;
	float AttackCooldown = 0.0f;
	float SwordAnimationTime = 0.0f;
	float SwordAnimationDuration = 0.18f;
	float SwordSwingDirection = -1.0f;
	FVector SwordSpriteRestLocation = FVector(8.0f, 0.0f, 0.0f);
};