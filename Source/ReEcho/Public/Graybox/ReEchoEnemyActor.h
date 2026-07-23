#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoEnemyActor.generated.h"

class UReEchoCombatantComponent;
class UStaticMeshComponent;
class AReEchoHealthBarActor;

UENUM(BlueprintType)
enum class EReEchoEnemyKind : uint8
{
	Grunt,
	Shield,
	Bomber,
	Boss
};

UCLASS()

class REECHO_API AReEchoEnemyActor : public AActor
{
	GENERATED_BODY()
public:
	AReEchoEnemyActor();
	virtual void Tick(float DeltaSeconds) override;
	void Configure(EReEchoEnemyKind InKind, int32 SpawnIndex);
	float ReceiveGrayboxDamage(float Damage, const FVector& SourceLocation);
	bool IsAlive() const;

	EReEchoEnemyKind GetKind() const
	{
		return Kind;
	}

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Shape;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoCombatantComponent> Combatant;
	UPROPERTY()
	TObjectPtr<AReEchoHealthBarActor> HealthBar;
	UPROPERTY()
	EReEchoEnemyKind Kind = EReEchoEnemyKind::Grunt;
	float MoveSpeed = 95.f;
	float ContactDamage = 9.f;
	float AttackInterval = 1.3f;
	float AttackCooldown = 0.f;
	float FuseRemaining = 0.f;
	float HitReactionRemaining = 0.f;
	FVector KnockbackVelocity = FVector::ZeroVector;
	FVector ShakeDirection = FVector::ZeroVector;
	FVector PreviousShakeOffset = FVector::ZeroVector;

	void ApplyVisual();
	void StartHitReaction(const FVector& SourceLocation);
	bool UpdateHitReaction(float DeltaSeconds);
};

