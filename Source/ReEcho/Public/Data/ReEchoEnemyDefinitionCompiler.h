#pragma once

#include "CoreMinimal.h"
#include "Enemies/ReEchoEnemyTypes.h"

struct FReEchoCsvDataSnapshot;

namespace ReEchoEnemyDefinitionCompiler
{
/** Main-module adapter from stable CSV ids into the resource-free ReEchoEnemies contract. */
REECHO_API bool Compile(const FReEchoCsvDataSnapshot& Snapshot,
                        FName EnemyId,
                        FReEchoEnemyDefinition& OutDefinition,
                        FString& OutError);
}
