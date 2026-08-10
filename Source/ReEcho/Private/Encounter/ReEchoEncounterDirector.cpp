#include "Encounter/ReEchoEncounterDirector.h"

#include "Core/ReEchoBalanceSettings.h"

AReEchoEncounterDirector::AReEchoEncounterDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AReEchoEncounterDirector::StartEncounter()
{
	EncounterTime = 0.f;
	Accumulator = 0.f;
	FixedDelta = 1.f / FMath::Max(1.f, GetDefault<UReEchoBalanceSettings>()->FixedStepHz);
	bSetupPhase = true;
	bRunning = true;
}

void AReEchoEncounterDirector::ResumeEncounter(const float SavedEncounterTime)
{
	EncounterTime = FMath::Clamp(
	    SavedEncounterTime, 0.0f, GetDefault<UReEchoBalanceSettings>()->EncounterDuration - KINDA_SMALL_NUMBER);
	Accumulator = 0.0f;
	FixedDelta = 1.0f / FMath::Max(1.0f, GetDefault<UReEchoBalanceSettings>()->FixedStepHz);
	bSetupPhase = EncounterTime < GetDefault<UReEchoBalanceSettings>()->SetupDuration;
	bRunning = true;
}

void AReEchoEncounterDirector::EndEncounter()
{
	if (!bRunning)
	{
		return;
	}
	bRunning = false;
	OnEncounterEnded.Broadcast();
}

float AReEchoEncounterDirector::GetRemainingTime() const
{
	return FMath::Max(0.f, GetDefault<UReEchoBalanceSettings>()->EncounterDuration - EncounterTime);
}

void AReEchoEncounterDirector::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRunning || bSimulationPaused)
	{
		return;
	}
	Accumulator += FMath::Min(DeltaSeconds, 0.25f);
	while (Accumulator >= FixedDelta && bRunning)
	{
		Accumulator -= FixedDelta;
		EncounterTime += FixedDelta;
		bSetupPhase = EncounterTime < GetDefault<UReEchoBalanceSettings>()->SetupDuration;
		OnFixedStep.Broadcast(FixedDelta);
		if (EncounterTime + KINDA_SMALL_NUMBER >= GetDefault<UReEchoBalanceSettings>()->EncounterDuration)
		{
			EncounterTime = GetDefault<UReEchoBalanceSettings>()->EncounterDuration;
			bRunning = false;
			OnEncounterEnded.Broadcast();
		}
	}
}

void AReEchoEncounterDirector::SetPaused(bool bInPaused)
{
	bSimulationPaused = bInPaused;
}
