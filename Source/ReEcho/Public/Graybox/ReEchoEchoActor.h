#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoEchoActor.generated.h"

class AReEchoTrajectoryActor;
class AReEchoWeaponActor;
class UBillboardComponent;
class UReEchoCombatantComponent;
class UReEchoCombatEventsComponent;
class UReEchoCombatAudioAdapterComponent;
class UReEchoPlaybackComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;
struct FReEchoCsvDataSnapshot;

/** 回响分身：使用录制构筑中的锁定武器，按固定时间轴重放历史位置与技能。 */
UCLASS()

class REECHO_API AReEchoEchoActor : public AActor, public IReEchoCombatAffiliation
{
	GENERATED_BODY()

public:
	AReEchoEchoActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 装载一份不可变录制，并设置回响造成伤害的效率倍率。 */
	bool InitializeEcho(const FReEchoRecording& Recording,
	                    float Efficiency,
	                    TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot);
	/** 将回响推进到指定遭遇时间，补播期间跨过的所有事件。 */
	void AdvanceEcho(float EncounterTime);
	/** 根据录制角色 ID 选择对应的回响形态。 */
	bool ConfigureEchoAppearance(FName CharacterId);
	/** 返回效率修正后的当前回响战斗属性。 */
	const FReEchoStatBlock& GetCurrentStats() const;
	float GetCurrentHealth() const;
	FString GetPinnedWeaponDomainRevision() const;
	FName GetEquippedWeaponId() const;
	FVector EvaluateRecordedPosition(float EncounterTime) const;
	virtual EReEchoCombatFaction GetCombatFaction() const override
	{
		return EReEchoCombatFaction::PlayerSide;
	}
	virtual EReEchoDamageSource GetCombatDamageSource() const override
	{
		return EReEchoDamageSource::Echo;
	}

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> CharacterSprite;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoPlaybackComponent> Playback;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatAudioAdapterComponent> CombatAudioAdapter;

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	UPROPERTY()
	TObjectPtr<AReEchoTrajectoryActor> Trajectory;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>> EchoTextures;

	FVector BaseSpriteLocation = FVector::ZeroVector;
	FVector BaseSpriteScale = FVector::OneVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float AutoTargetRange = 1600.0f;
	float DamageEfficiency = 1.0f;
	bool bAudioLifecycleStarted = false;
};
