#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoEchoActor.generated.h"

class AReEchoTrajectoryActor;
class AReEchoWeaponActor;
class UBillboardComponent;
class UReEchoCombatantComponent;
class UReEchoPlaybackComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;

/** 回响分身：按固定时间轴重放历史位置、技能和武器事件。 */
UCLASS()

class REECHO_API AReEchoEchoActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoEchoActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 装载一份不可变录制，并设置回响造成伤害的效率倍率。 */
	void InitializeEcho(const FReEchoRecording& Recording, float Efficiency);
	/** 将回响推进到指定遭遇时间，补播期间跨过的所有事件。 */
	void AdvanceEcho(float EncounterTime);
	/** 根据录制角色 ID 选择对应的回响形态。 */
	bool ConfigureEchoAppearance(FName CharacterId);

private:
	UFUNCTION()
	void HandleReplayedWeapon(FName WeaponId, float RecordedTime);

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
};
