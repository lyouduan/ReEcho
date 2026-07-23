#include "Graybox/ReEchoAttackEffects.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
UNiagaraSystem* LoadEffect(const TCHAR* Path)
{
	return LoadObject<UNiagaraSystem>(nullptr, Path);
}

void ApplyColor(UNiagaraComponent* Component, const FLinearColor& Color)
{
	if (!Component)
	{
		return;
	}
	Component->SetVariableLinearColor(TEXT("User.Color"), Color);
	Component->SetVariableLinearColor(TEXT("User.TintColor"), Color);
}
}

void ReEchoAttackEffects::SpawnHitImpact(UWorld* World, const FVector& Location)
{
	if (!World)
	{
		return;
	}
	if (UNiagaraSystem* Impact =
	        LoadEffect(TEXT("/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion.SimpleExplosion")))
	{
		ApplyColor(
		    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		        World, Impact, Location + FVector(0.f, 0.f, 35.f), FRotator::ZeroRotator, FVector(0.38f), true, true),
		    FLinearColor(1.f, 0.03f, 0.01f));
	}
}

