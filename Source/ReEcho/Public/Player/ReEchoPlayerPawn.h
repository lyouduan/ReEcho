#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "GameFramework/Pawn.h"
#include "ReEchoPlayerPawn.generated.h"

class AReEchoEnemyActor;
class AReEchoWeaponActor;
class UAbilitySystemComponent;
class UBillboardComponent;
class UCameraComponent;
class UCapsuleComponent;
class UFloatingPawnMovement;
class UGameplayAbility;
class UPaperFlipbook;
class UReEchoCombatAttributeSet;
class UReEchoCombatantComponent;
class UReEcho2DAnimationComponent;
class UReEchoRecorderComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;

struct FGameplayTag;
struct FOnAttributeChangeData;
struct FReEchoBuildSnapshot;
struct FReEchoCsvDataSnapshot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoActiveSkill, FVector, Position, FName, SkillId);

/** 自动攻击确定性目标选择用的瞬时 C++ 候选信息。 */
struct FReEchoAttackTargetCandidate
{
	FVector Location = FVector::ZeroVector;
	bool bAlive = false;
	int32 StableId = 0;
	AReEchoEnemyActor* Source = nullptr;
};

/** 玩家可控角色：组合移动、GAS 技能、武器、录制以及 2D 序列帧表现。 */
UCLASS(Blueprintable)

class REECHO_API AReEchoPlayerPawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AReEchoPlayerPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FString GetEquippedWeaponLabel() const;
	float GetCurrentAttackInterval() const;
	/** 由 GameplayAbility 回调，执行当前武器的基础攻击。 */
	bool ExecuteBasicAttackAbility();
	bool ExecuteActiveAttackAbility();
	void PlayHitVisual();
	bool IsWeaponInvulnerable() const;
	/** 切换玩家角色外观；未知 ID 会保留当前角色。 */
	bool ConfigureCharacter(FName CharacterId);
	/** Synchronize the spawned weapon actor with a restored build without recording a new switch event. */
	void RestoreEquippedWeapon(FName WeaponId);
	bool InitializeWeaponFromBuild(const FReEchoBuildSnapshot& Build,
	                               TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot);
	FString GetPinnedWeaponDomainRevision() const;
	/** 设置与当前场景尺寸一致的玩家活动半径：X对应场景高度，Y对应场景宽度。 */
	void ConfigureArenaBounds(float HalfExtentX, float HalfExtentY);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> VisualEffectRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBillboardComponent> CharacterSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEcho2DAnimationComponent> SequenceAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UReEchoCombatAttributeSet> CombatAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoRecorderComponent> Recorder;

	UPROPERTY(BlueprintAssignable)
	FReEchoActiveSkill OnActiveSkill;

	/** 当前是否使用自动普通攻击（默认 true）。 */
	bool IsAutoAttackMode() const { return bAutoAttackMode; }
	/** 设置攻击模式。切换到手动模式时会释放模拟的 held basic-attack input。 */
	void SetAutoAttackMode(bool bAuto);
	/** 模拟的 held basic-attack input 当前是否被按下（自动模式驱动）。用于测试与内部清理。 */
	bool IsAutoAttackInputHeld() const { return bAutoAttackInputHeld; }
	/** 只读访问录像组件，供确定性测试验证攻击/模式切换不产生录像事件。 */
	UReEchoRecorderComponent* GetRecorder() const { return Recorder; }
	/** 按下模拟的 held basic-attack input（自动攻击驱动）。 */
	void PressAutoAttackInput();
	/** 释放模拟的 held basic-attack input（菜单/死亡/切换手动/无目标时调用）。 */
	void ReleaseAutoAttackInput();
	/** 物理（手动）普通攻击输入按下。仅在手动模式下驱动 GAS basic-attack 输入；自动模式下忽略（输入源缺陷修复）。 */
	void ManualBasicAttack();
	/** 物理（手动）普通攻击输入抬起。仅在手动模式下释放 GAS basic-attack 输入；自动模式下忽略。 */
	void ManualStopBasicAttack();
	/** 物理（手动）普通攻击输入当前是否被按下（手动模式驱动），用于测试与内部清理。 */
	bool IsManualAttackInputHeld() const { return bManualAttackInputHeld; }
	/** 释放所有 held basic-attack 输入源（自动循环 + 物理），用于菜单开/关时统一清理。 */
	void ReleaseAllBasicAttackInputs();
	/** 确定性目标选择：返回射程内最近的存活敌人索引；无目标返回 INDEX_NONE。相同距离按 StableId 升序打破平局。 */
	static int32 SelectNearestEnemyInRange(const FVector& Origin, float RangeCm, TArrayView<const FReEchoAttackTargetCandidate> Candidates);

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void ActivateSkill();
	void BasicAttack();
	void StopBasicAttack();
	/** 自动攻击每帧驱动：寻找射程内最近存活敌人，并幂等保持 GAS basic-attack 输入。 */
	void UpdateAutoAttack(bool& bOutHasTarget);
	/** 在给定射程内寻找最近的存活敌人指针（无则 null）。 */
	AReEchoEnemyActor* FindNearestEnemyInRange(float RangeCm) const;
	void TogglePauseMenu();
	void ToggleInventoryMenu();
	void ToggleShopMenu();
	void ToggleStatsMenu();
	void ConfigureMouseInput();
	/** 将鼠标位置投射到战斗平面，并据此更新角色左右朝向。 */
	void UpdateMouseAim();
	void ConstrainToArenaBounds();
	void UpdateFollowCamera();
	void GrantStartupAbilities();
	void AbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);
	void StartAttackVisual(float Duration, float Strength);
	void UpdateSpriteAnimation(float DeltaSeconds);
	/** 根据当前动画状态选择并显示对应的角色序列帧。 */
	void UpdateSequenceFrame();
	void HandleMovementSpeedAttributeChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	UPROPERTY()
	TObjectPtr<UPaperFlipbook> SpadeIdleFlipbook;

	bool bMouseInputConfigured = false;
	bool bAutoAttackMode = true;
	bool bAutoAttackInputHeld = false;
	bool bManualAttackInputHeld = false;
	FVector2D ArenaHalfExtents = FVector2D::ZeroVector;
	FVector BaseVisualLocation = FVector::ZeroVector;
	FVector BaseVisualScale = FVector::OneVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float AttackVisualDuration = 0.0f;
	float AttackVisualStrength = 0.0f;
	float HitVisualRemaining = 0.0f;
	float VisualFacingSign = 1.0f;
	float AppliedVisualFacingSign = 0.0f;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>> CharacterTextures;

	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> IdleAnimationFrames;

	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> AttackAnimationFrames;
};
