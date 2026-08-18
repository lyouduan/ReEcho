#pragma once

#include "AbilitySystemInterface.h"
#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoAttackHost.h"
#include "Combat/ReEchoCombatTarget.h"
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "GameFramework/Pawn.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTypes.h"
#include "ReEchoPlayerPawn.generated.h"

class AReEchoEnemyActor;
class AReEchoWeaponActor;
class UAbilitySystemComponent;
class UBoxComponent;
class UFloatingPawnMovement;
class UGameplayAbility;
class UPaperFlipbook;
class UReEchoCombatAttributeSet;
class UReEchoCombatantComponent;
class UReEchoCombatEventsComponent;
class UReEchoCombatAudioAdapterComponent;
class UReEchoCombatVfxComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DPresentationCatalog;
class UReEcho2DPresentationController;
class UReEcho2DFrameCollisionDriver;
class UReEcho2DSceneLightingComponent;
class UMaterialInterface;
class UReEchoRecorderComponent;
class UReEchoTargetingComponent;
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

class REECHO_API AReEchoPlayerPawn : public APawn,
                                     public IAbilitySystemInterface,
                                     public IReEchoAttackHost,
                                     public IReEchoAttackControllerHost,
                                     public IReEchoCombatTarget,
                                     public IReEchoCombatAffiliation
{
	GENERATED_BODY()

public:
	AReEchoPlayerPawn();
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual bool IsCombatTargetAlive() const override;
	virtual FVector GetCombatTargetLocation() const override;
	virtual int32 GetCombatTargetTieBreakIndex() const override;
	virtual UReEchoCombatantComponent* GetCombatTargetCombatant() const override;

	virtual EReEchoCombatFaction GetCombatFaction() const override
	{
		return EReEchoCombatFaction::PlayerSide;
	}

	virtual EReEchoDamageSource GetCombatDamageSource() const override
	{
		return EReEchoDamageSource::Player;
	}

	virtual bool
	IntersectsCombatPath(const FVector& PathStart, const FVector& PathEnd, float CarrierRadius) const override;
	virtual float ModifyIncomingRawDamage(const FReEchoHitIntent& Intent) const override;
	virtual void ModifyOutgoingHit(FReEchoHitIntent& Intent) const override;
	virtual void NotifyReactionResolved(FName ReactionId) const override;
	virtual void NotifyKillResolved() const override;

	FString GetEquippedWeaponLabel() const;
	float GetCurrentAttackInterval() const;
	/** 武器是否处于临时动作锁（有序攻击步骤锁）中。held 普攻循环据此决定是否重试而非终止。 */
	bool IsWeaponActionLocked() const;
	/** 武器动作锁剩余秒数。 */
	float GetWeaponActionLockRemaining() const;

	/** 只读访问当前武器执行器，供确定性测试观察攻击计数等。 */
	AReEchoWeaponActor* GetWeapon() const
	{
		return Weapon;
	}

	/** 由 GameplayAbility 回调，执行当前武器的基础攻击。 */
	bool ExecuteBasicAttackAbility();
	bool ExecuteActiveAttackAbility();
	virtual EReEchoAttackAttempt TryCommitBasicAttack() override;
	virtual float GetBasicAttackWaitRemaining() const override;
	virtual bool ExecuteActiveAttack() override;
	virtual float GetActiveAttackCooldown() const override;
	virtual bool CanIssueAttackRequest() const override;
	virtual float GetAutomaticAttackRange() const override;
	virtual void FaceAutomaticTarget(AActor& Target) override;
	virtual void PressBasicAttackInput() override;
	virtual void ReleaseBasicAttackInput() override;
	virtual FName GetAttackWeaponId() const override;
	virtual FName GetAttackStepId() const override;
	virtual float GetAttackReadinessRemaining() const override;
	virtual float GetEffectiveAttackSpeed() const override;
	void PlayHitVisual();
	bool IsWeaponInvulnerable() const;
	/** 切换玩家角色外观；未知 ID 会保留当前角色。 */
	bool ConfigureCharacter(FName CharacterId);

	UTexture2D* GetPortraitTexture() const
	{
		return PortraitTexture;
	}

	/** 当前明确的 2D 表现状态：静止 Idle、移动 Walk、攻击 Attack。 */
	/** Synchronize the spawned weapon actor with a restored build without recording a new switch event. */
	void RestoreEquippedWeapon(FName WeaponId);
	bool InitializeWeaponFromBuild(const FReEchoBuildSnapshot& Build,
	                               TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot);
	FString GetPinnedWeaponDomainRevision() const;
	/** 设置 Editor 场景声明的玩家活动中心与半径。 */
	void ConfigureArenaBounds(const FVector2D& Center, const FVector2D& HalfExtents);
	void ConfigureArenaBounds(float HalfExtentX, float HalfExtentY);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Collision")
	TObjectPtr<UBoxComponent> Collision;

	/** Blueprint-authored overall visual offset. Runtime animation is applied additively from this transform. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Presentation")
	TObjectPtr<USceneComponent> PresentationRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Ground")
	TObjectPtr<USceneComponent> FootRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Presentation")
	TObjectPtr<USceneComponent> PresentationMotionRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Flipbook")
	TObjectPtr<USceneComponent> FlipbookRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Ground")
	TObjectPtr<USceneComponent> GroundRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Effects")
	TObjectPtr<USceneComponent> EffectsRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Ground")
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character Scene|Flipbook")
	TObjectPtr<UReEcho2DAnimationComponent> SequenceAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEcho2DPresentationController> PresentationController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEcho2DFrameCollisionDriver> FrameCollisionDriver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEcho2DSceneLightingComponent> SceneLighting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UReEchoCombatAttributeSet> CombatAttributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UReEchoAttackControllerComponent> AttackController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UReEchoTargetingComponent> Targeting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UReEchoCombatAudioAdapterComponent> CombatAudioAdapter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UReEchoCombatVfxComponent> CombatVfx;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoRecorderComponent> Recorder;

	UPROPERTY(BlueprintAssignable)
	FReEchoActiveSkill OnActiveSkill;

	/** 当前是否使用自动普通攻击（默认 true）。 */
	bool IsAutoAttackMode() const;
	/** 设置攻击模式。切换到手动模式时会释放模拟的 held basic-attack input。 */
	void SetAutoAttackMode(bool bAuto);

	/** 模拟的 held basic-attack input 当前是否被按下（自动模式驱动）。用于测试与内部清理。 */
	bool IsAutoAttackInputHeld() const;

	/** 只读访问录像组件，供确定性测试验证攻击/模式切换不产生录像事件。 */
	UReEchoRecorderComponent* GetRecorder() const
	{
		return Recorder;
	}

	/** 按下模拟的 held basic-attack input（自动攻击驱动）。 */
	void PressAutoAttackInput();
	/** 释放模拟的 held basic-attack input（菜单/死亡/切换手动/无目标时调用）。 */
	void ReleaseAutoAttackInput();
	/** 物理（手动）普通攻击输入按下。仅在手动模式下驱动 GAS basic-attack 输入；自动模式下忽略（输入源缺陷修复）。 */
	void ManualBasicAttack();
	/** 物理（手动）普通攻击输入抬起。仅在手动模式下释放 GAS basic-attack 输入；自动模式下忽略。 */
	void ManualStopBasicAttack();

	/** 物理（手动）普通攻击输入当前是否被按下（手动模式驱动），用于测试与内部清理。 */
	bool IsManualAttackInputHeld() const;
	/** 释放所有 held basic-attack 输入源（自动循环 + 物理），用于菜单开/关时统一清理。 */
	void ReleaseAllBasicAttackInputs();
	/** 确定性目标选择：返回射程内最近的存活敌人索引；无目标返回 INDEX_NONE。相同距离按 StableId 升序打破平局。 */
	static int32 SelectNearestEnemyInRange(const FVector& Origin,
	                                       float RangeCm,
	                                       TArrayView<const FReEchoAttackTargetCandidate> Candidates);

	FVector GetAttackAimDirection() const
	{
		return AttackAimDirection;
	}

protected:
	/** Uniform Blueprint-authored scale for the complete gameplay actor, including collision and presentation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.01"))
	float CharacterScale = 1.0f;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
	void GrantStartupAbilities();
	void AbilityInputPressed(const FGameplayTag& InputTag);
	void AbilityInputReleased(const FGameplayTag& InputTag);
	void StartAttackVisual(float Duration, float Strength);
	void UpdateSpriteAnimation(float DeltaSeconds);
	void RefreshPresentationProfile();
	void RefreshWeaponPresentationSet();
	void ApplyPresentationMotion(const FVector& Offset, const FVector& Scale);
	/** 根据当前动画状态选择并显示对应的角色序列帧。 */
	void UpdateSequenceFrame();
	void RefreshFootRoot();
	void HandleMovementSpeedAttributeChanged(const FOnAttributeChangeData& Data);

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	UPROPERTY()
	TObjectPtr<UReEcho2DPresentationCatalog> PresentationCatalog;
	UPROPERTY(Transient)
	TObjectPtr<UReEcho2DCharacterPresentationProfile> ActivePresentationProfile;
	FName CurrentCharacterId;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PortraitTexture;

	bool bMouseInputConfigured = false;
	FVector2D ArenaCenter = FVector2D::ZeroVector;
	FVector2D ArenaHalfExtents = FVector2D::ZeroVector;
	FVector BaseVisualLocation = FVector::ZeroVector;
	FVector BaseVisualScale = FVector::OneVector;
	FVector BaseMotionLocation = FVector::ZeroVector;
	FVector BaseEffectsLocation = FVector::ZeroVector;
	FVector BaseEffectsScale = FVector::OneVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float AttackVisualDuration = 0.0f;
	float AttackVisualStrength = 0.0f;
	float HitVisualRemaining = 0.0f;
	float VisualFacingSign = 1.0f;
	FVector AttackAimDirection = FVector::ForwardVector;
	int64 NextPresentationAttackInstanceId = 1;

	UPROPERTY()
	TMap<FName, TObjectPtr<UTexture2D>> CharacterTextures;
};
