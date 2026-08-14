#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ReEchoAttackHost.generated.h"

UENUM()
enum class EReEchoAttackAttempt : uint8
{
	Committed,
	Waiting,
	Rejected
};

UINTERFACE(MinimalAPI)

class UReEchoAttackHost : public UInterface
{
	GENERATED_BODY()
};

/** Narrow bridge implemented by a gameplay avatar; Combat never depends on the concrete Pawn or Weapon. */
class REECHOCOMBAT_API IReEchoAttackHost
{
	GENERATED_BODY()

public:
	virtual EReEchoAttackAttempt TryCommitBasicAttack() = 0;
	virtual float GetBasicAttackWaitRemaining() const = 0;
	virtual bool ExecuteActiveAttack() = 0;
	virtual float GetActiveAttackCooldown() const = 0;
};
