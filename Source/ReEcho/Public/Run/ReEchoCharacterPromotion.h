#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"

namespace ReEchoCharacterPromotion
{
REECHO_API FName EvaluateRole(const TArray<FName>& Cards);
REECHO_API bool TryPromote(FReEchoBuildSnapshot& Build);
REECHO_API bool IsRole(const FReEchoBuildSnapshot& Build, FName RoleId);
}
