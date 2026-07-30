#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
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
class UReEchoCombatantComponent;
class UReEchoRecorderComponent;
class UStaticMeshComponent;
class UTexture2D;

enum class EReEchoWeaponSlot : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoActiveSkill, FVector, Position, FName, SkillId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoWeaponChanged, FName, WeaponId);

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
	/** 由 GameplayAbility 回调，执行当前武器的基础攻击。 */
	bool ExecuteBasicAttackAbility();
	bool ExecuteActiveAttackAbility();
	bool ExecuteSelectWeaponSlot1Ability();
	bool ExecuteSelectWeaponSlot2Ability();
	bool ExecuteSelectWeaponSlot3Ability();
	void PlayHitVisual();
	/** 切换玩家角色外观；未知 ID 会保留当前角色。 */
	bool ConfigureCharacter(FName CharacterId);
	/** 设置与当前场景尺寸一致的玩家活动半径：X对应场景高度，Y对应场景宽度。 */
	void ConfigureArenaBounds(float HalfExtentX, float HalfExtentY);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> GroundShadow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UBillboardComponent> CharacterSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UFloatingPawnMovement> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoCombatantComponent> Combatant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UReEchoRecorderComponent> Recorder;

	UPROPERTY(BlueprintAssignable)
	FReEchoActiveSkill OnActiveSkill;

	UPROPERTY(BlueprintAssignable)
	FReEchoWeaponChanged OnWeaponChanged;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void ActivateSkill();
	void BasicAttack();
	void StopBasicAttack();
	void TogglePauseMenu();
	void ToggleInventoryMenu();
	void ToggleShopMenu();
	void ToggleStatsMenu();
	void SelectWeaponSlot1();
	void SelectWeaponSlot2();
	void SelectWeaponSlot3();
	void ConfigureMouseInput();
	/** 将鼠标位置投射到战斗平面，并据此更新角色左右朝向。 */
	void UpdateMouseAim();
	void ConstrainToArenaBounds();
	void UpdateFollowCamera();
	void GrantStartupAbilities();
	bool TryActivatePlayerAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	bool ExecuteSelectWeaponAbility(EReEchoWeaponSlot WeaponSlot, FName WeaponId);
	void StartAttackVisual(float Duration, float Strength);
	void UpdateSpriteAnimation(float DeltaSeconds);
	/** 根据当前动画状态选择并显示对应的角色序列帧。 */
	void UpdateSequenceFrame();

	UPROPERTY()
	TObjectPtr<AReEchoWeaponActor> Weapon;

	bool bMouseInputConfigured = false;
	FVector2D ArenaHalfExtents = FVector2D::ZeroVector;
	bool bBasicAttackHeld = false;
	FVector BaseSpriteLocation = FVector::ZeroVector;
	FVector BaseSpriteScale = FVector::OneVector;
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