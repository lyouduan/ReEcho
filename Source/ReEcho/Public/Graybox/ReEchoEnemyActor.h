#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"
#include "GameFramework/Actor.h"
#include "ReEchoEnemyActor.generated.h"

class UAbilitySystemComponent;
class UBillboardComponent;
class UCapsuleComponent;
class UReEchoCombatAttributeSet;
class UReEchoCombatantComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UReEcho2DAnimationComponent;
class UReEcho2DCharacterPresentationProfile;
class UReEcho2DPresentationController;
class USceneComponent;
class UTexture2D;
class AReEchoHealthBarActor;

UENUM(BlueprintType)
enum class EReEchoEnemyKind : uint8
{
	Grunt,
	Shield,
	Bomber,
	Boss
};

/** 2D 敌人实体：负责寻路攻击、受伤判定、血条和序列帧表现。 */
UCLASS()

class REECHO_API AReEchoEnemyActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AReEchoEnemyActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/** 按敌人类型和出生序号装配数值、贴图、碰撞体及行为参数。 */
	void Configure(EReEchoEnemyKind InKind, int32 SpawnIndex);
	/** 结算伤害并触发受击方向反馈，返回实际扣除的生命值。 */
	float ReceiveGrayboxDamage(float Damage,
	                           const FVector& SourceLocation,
	                           AActor* SourceActor = nullptr,
	                           const FLinearColor& DamageNumberColor = FLinearColor::White);
	/** Applies an elemental attachment or a supported two-element reaction before regular damage resolution. */
	float ReceiveElementalDamage(float Damage,
	                             EReEchoElement Element,
	                             const FVector& SourceLocation,
	                             AActor* SourceActor = nullptr,
	                             float ReactionEfficiency = 1.0f);
	void RefreshElementAttachmentVisual();

	EReEchoElement GetAttachedElement() const
	{
		return ElementState.Attached;
	}

	const FReEchoElementState& GetElementState() const
	{
		return ElementState;
	}

	FReEchoElementState& EditElementState()
	{
		return ElementState;
	}

	UReEchoCombatantComponent* GetCombatantComponent() const
	{
		return Combatant;
	}

	bool IsAlive() const;
	/** 稳定的运行时目标排序标识（等于生成索引）。供自动攻击在射程内出现相同距离时确定性打破平局。 */
	int32 GetSpawnIndex() const { return VisualVariantIndex; }
	bool IntersectsProjectilePath(const FVector& PathStart, const FVector& PathEnd, float ProjectileRadius) const;

	EReEchoEnemyKind GetKind() const
	{
		return Kind;
	}

	FReEchoEnemyRuntimeState CaptureRuntimeState() const;
	void RestoreRuntimeState(const FReEchoEnemyRuntimeState& SavedState);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(VisibleAnywhere, Category = "Abilities")
	TObjectPtr<UReEchoCombatAttributeSet> CombatAttributes;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Collision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundShadow;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> VisualEffectRoot;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> CharacterSprite;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DAnimationComponent> SequenceAnimation;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEcho2DPresentationController> PresentationController;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> ElementAuraRing;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> ElementAttachmentLabel;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> ElementAuraLight;
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> GruntTextures;
	UPROPERTY()
	TObjectPtr<UTexture2D> BossTexture;
	UPROPERTY()
	TObjectPtr<UReEcho2DCharacterPresentationProfile> GruntPresentationProfile;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;
	UPROPERTY()
	TObjectPtr<AReEchoHealthBarActor> HealthBar;
	UPROPERTY()
	EReEchoEnemyKind Kind = EReEchoEnemyKind::Grunt;
	FReEchoElementState ElementState;
	int32 VisualVariantIndex = 0;
	float MoveSpeed = 95.f;
	float ContactDamage = 9.f;
	float AttackInterval = 1.3f;
	float AttackCooldown = 0.f;
	float FuseRemaining = 0.f;
	bool bBomberFuseActive = false;
	float HitReactionRemaining = 0.f;
	FVector KnockbackVelocity = FVector::ZeroVector;
	FVector ShakeDirection = FVector::ZeroVector;
	FVector PreviousShakeOffset = FVector::ZeroVector;
	FVector BaseVisualLocation = FVector::ZeroVector;
	FVector BaseVisualScale = FVector::OneVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float DeathVisualRemaining = 0.0f;

	void ApplyVisual();
	void StartHitReaction(const FVector& SourceLocation);
	void UpdateElementAttachmentVisual();
	void UpdateElementAttachmentFacing();
	bool UpdateHitReaction(float DeltaSeconds);
	void StartAttackVisual();
	void UpdateSpriteAnimation(float DeltaSeconds, bool bMoving);
	void UpdateDeathAnimation(float DeltaSeconds);
};
