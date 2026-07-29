#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoEnemyActor.generated.h"

class UBillboardComponent;
class UCapsuleComponent;
class UReEchoCombatantComponent;
class UStaticMeshComponent;
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

class REECHO_API AReEchoEnemyActor : public AActor
{
	GENERATED_BODY()
public:
	AReEchoEnemyActor();
	virtual void Tick(float DeltaSeconds) override;
	/** 按敌人类型和出生序号装配数值、贴图、碰撞体及行为参数。 */
	void Configure(EReEchoEnemyKind InKind, int32 SpawnIndex);
	/** 结算伤害并触发受击方向反馈，返回实际扣除的生命值。 */
	float ReceiveGrayboxDamage(float Damage, const FVector& SourceLocation);
	bool IsAlive() const;
	bool IntersectsProjectilePath(const FVector& PathStart, const FVector& PathEnd, float ProjectileRadius) const;

	EReEchoEnemyKind GetKind() const
	{
		return Kind;
	}

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Collision;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GroundShadow;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBillboardComponent> CharacterSprite;
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> GruntTextures;
	UPROPERTY()
	TObjectPtr<UTexture2D> BossTexture;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;
	UPROPERTY()
	TObjectPtr<AReEchoHealthBarActor> HealthBar;
	UPROPERTY()
	EReEchoEnemyKind Kind = EReEchoEnemyKind::Grunt;
	int32 VisualVariantIndex = 0;
	float MoveSpeed = 95.f;
	float ContactDamage = 9.f;
	float AttackInterval = 1.3f;
	float AttackCooldown = 0.f;
	float FuseRemaining = 0.f;
	float HitReactionRemaining = 0.f;
	FVector KnockbackVelocity = FVector::ZeroVector;
	FVector ShakeDirection = FVector::ZeroVector;
	FVector PreviousShakeOffset = FVector::ZeroVector;
	FVector BaseSpriteLocation = FVector::ZeroVector;
	FVector BaseSpriteScale = FVector::OneVector;
	float VisualTime = 0.0f;
	float AttackVisualRemaining = 0.0f;
	float DeathVisualRemaining = 0.0f;

	void ApplyVisual();
	void StartHitReaction(const FVector& SourceLocation);
	bool UpdateHitReaction(float DeltaSeconds);
	void StartAttackVisual();
	void UpdateSpriteAnimation(float DeltaSeconds, bool bMoving);
	void UpdateDeathAnimation(float DeltaSeconds);
};
