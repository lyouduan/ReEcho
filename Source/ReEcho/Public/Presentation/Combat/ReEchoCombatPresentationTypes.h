#pragma once

#include "Combat/ReEchoCombatTypes.h"
#include "CoreMinimal.h"
#include "ReEchoCombatPresentationTypes.generated.h"

UENUM(BlueprintType)
enum class EReEchoPresentationActionPhase : uint8
{
	Windup,
	Committed,
	Travel,
	Recovery,
	Ended,
	Cancelled
};

UENUM(BlueprintType)
enum class EReEchoPresentationHostKind : uint8
{
	Player,
	Echo,
	OrdinaryEnemy,
	Boss
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoPresentationActionKey
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<AActor> Source = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int64 Sequence = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName AbilityId = NAME_None;

	bool IsValid() const
	{
		return Source != nullptr && !AbilityId.IsNone();
	}

	bool operator==(const FReEchoPresentationActionKey& Other) const
	{
		return Source == Other.Source && Sequence == Other.Sequence && AbilityId == Other.AbilityId;
	}
};

USTRUCT(BlueprintType)

struct REECHO_API FReEchoPresentationActionEvent
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoPresentationActionKey Key;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoPresentationActionPhase Phase = EReEchoPresentationActionPhase::Windup;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FReEchoAttackIdentity Attack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector Origin = FVector::ZeroVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector LockedDirection = FVector::ForwardVector;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FVector LockedTargetLocation = FVector::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoPresentationActionDelegate,
                                            const FReEchoPresentationActionEvent&,
                                            Event);
