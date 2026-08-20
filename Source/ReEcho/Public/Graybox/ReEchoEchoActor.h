#pragma once

#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Core/ReEchoTypes.h"
#include "Combat/ReEchoCombatTarget.h"
#include "GameFramework/Actor.h"
#include "ReEchoEchoActor.generated.h"

class AReEchoWeaponActor;
class UBillboardComponent;
class UReEchoCombatantComponent;
class UReEchoCombatEventsComponent;
class UReEchoCombatAudioAdapterComponent;
class UReEchoCombatVfxComponent;
class UReEchoPlaybackComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;
struct FReEchoCsvDataSnapshot;

/** 回响分身：使用录制构筑中的锁定武器，按固定时间轴重放历史位置与技能。 */
UCLASS()

class REECHO_API AReEchoEchoActor : public AActor, public IReEchoCombatTarget, public IReEchoCombatAffiliation
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
	void ConfigureCardRules(const FReEchoCardRuleSnapshot& Rules, const FReEchoStatBlock& PlayerStats);
	virtual bool IsCombatTargetAlive() const override;

	virtual FVector GetCombatTargetLocation() const override
	{
		return GetActorLocation();
	}

	virtual int32 GetCombatTargetTieBreakIndex() const override
	{
		return GetUniqueID();
	}

	virtual UReEchoCombatantComponent* GetCombatTargetCombatant() const override
	{
		return Combatant;
	}

	virtual bool
	IntersectsCombatPath(const FVector& PathStart, const FVector& PathEnd, float CarrierRadius) const override;
	virtual void ModifyOutgoingHit(FReEchoHitIntent& Intent) const override;
	virtual void NotifyReactionResolved(FName ReactionId) const override;
	virtual void NotifyKillResolved() const override;
	virtual void NotifyDefeated(EReEchoDamageSource DamageSource) const override;
	FString GetPinnedWeaponDomainRevision() const;
	FName GetEquippedWeaponId() const;
	FVector EvaluateRecordedPosition(float EncounterTime) const;

	/** 返回录制的历史位置（世界 XY 平面），供右上角小地图绘制回响轨迹。 */
	const TArray<FVector2D>& GetRecordedPath() const { return RecordedPath; }

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
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> EffectsRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> AttackVfxRoot;
	UPROPERTY(VisibleAnywhere,
	          BlueprintReadOnly,
	          Category = "Character Scene|Effects",
	          meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HurtVfxRoot;

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

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatVfxComponent> CombatVfx;

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	UPROPERTY()
	TArray<FVector2D> RecordedPath;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>> EchoTextures;

	FVector BaseSpriteLocation = FVector::ZeroVector;
	FVector BaseSpriteScale = FVector::OneVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float AutoTargetRange = 1600.0f;
	float DamageEfficiency = 1.0f;
	bool bAudioLifecycleStarted = false;
	bool bCanAttack = true;
};
