#include "ReEchoGameMode.h"

#include "Camera/CameraComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Enemies/ReEchoEnemyRosterComponent.h"
#include "Enemies/ReEchoEnemyLogicComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Presentation/Scene/ReEchoArenaSceneActor.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
constexpr int32 OpeningCrowdSectors = 8;

int32 OpeningCrowdSectorQuota(const int32 Total, const int32 Sector)
{
	return Total / OpeningCrowdSectors + (Sector < Total % OpeningCrowdSectors ? 1 : 0);
}

FVector SampleOpeningCrowdOffset(FRandomStream& Random, const int32 Sector, const float Rotation, const float Radius)
{
	const float Angle = Rotation + (Sector + Random.FRand()) * (2.0f * PI / OpeningCrowdSectors);
	const float Distance = Radius * FMath::Sqrt(Random.FRandRange(0.09f, 1.0f));
	return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
}
}

void AReEchoGameMode::PrepareBossSacrifice()
{
	const AReEchoEnemyActor* Boss = TransformingBoss.Get();
	const UCameraComponent* Camera = ArenaCameraActor ? ArenaCameraActor->ArenaCamera.Get() : nullptr;
	if (!Boss || !Camera)
	{
		return;
	}
	const UReEchoBossPhase3Config* Config =
	    BossPhase3Config ? BossPhase3Config.Get() : GetDefault<UReEchoBossPhase3Config>();
	const bool bOpeningCrowd = BossTransformTargetPhase == 3;
	if (bOpeningCrowd && Boss->GetEnemyLogicComponent()->GetSnapshot().bBossOpeningConsumed)
	{
		return;
	}
	TArray<AReEchoEnemyActor*> Candidates;
	for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It; ++It)
	{
		AReEchoEnemyActor* Enemy = *It;
		if (Enemy == Boss || !Enemy->IsAlive() || Enemy->GetKind() == EReEchoEnemyKind::Boss ||
		    Enemy->GetKind() == EReEchoEnemyKind::Elite || Enemy->IsHidden())
		{
			continue;
		}
		UReEchoEnemyPresentationComponent* Visual = Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>();
		if (!Visual || FVector::DistSquared2D(Enemy->GetActorLocation(), Boss->GetActorLocation()) >
		                   FMath::Square(FMath::Max(0.0f, Config->SacrificeRadiusCm)))
		{
			continue;
		}
		const FVector ToCenter = Visual->GetSacrificeVisualCenter() - Camera->GetComponentLocation();
		const float HalfWidth = Camera->OrthoWidth * 0.5f;
		UE_LOG(LogTemp,
		       Verbose,
		       TEXT("[BossSacrifice] candidate=%s center=%s depth=%.1f right=%.1f up=%.1f halfWidth=%.1f"),
		       *GetNameSafe(Enemy),
		       *Visual->GetSacrificeVisualCenter().ToString(),
		       FVector::DotProduct(ToCenter, Camera->GetForwardVector()),
		       FVector::DotProduct(ToCenter, Camera->GetRightVector()),
		       FVector::DotProduct(ToCenter, Camera->GetUpVector()),
		       HalfWidth);
		if (FVector::DotProduct(ToCenter, Camera->GetForwardVector()) <= 0.0f ||
		    FMath::Abs(FVector::DotProduct(ToCenter, Camera->GetRightVector())) > HalfWidth ||
		    FMath::Abs(FVector::DotProduct(ToCenter, Camera->GetUpVector())) >
		        HalfWidth / FMath::Max(0.1f, Camera->AspectRatio))
		{
			continue;
		}
		Candidates.Add(Enemy);
	}
	Candidates.Sort(
	    [Boss](const AReEchoEnemyActor& A, const AReEchoEnemyActor& B)
	    {
		    const float DA = FVector::DistSquared2D(A.GetActorLocation(), Boss->GetActorLocation());
		    const float DB = FVector::DistSquared2D(B.GetActorLocation(), Boss->GetActorLocation());
		    return DA == DB ? A.GetSpawnIndex() < B.GetSpawnIndex() : DA < DB;
	    });
	for (AReEchoEnemyActor* Enemy : Candidates)
	{
		if (bOpeningCrowd)
		{
			break; // Existing actors remain on the ground; only new summons need Born tracking.
		}
		if (BossSacrificeEnemies.Num() >= FMath::Clamp(Config->SacrificeMaxEnemies, 0, 8))
		{
			break;
		}
		UReEchoEnemyPresentationComponent* Visual = Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>();
		if (Visual)
		{
			BossSacrificeEnemies.Add(Enemy);
		}
	}
	// Fixed extra cinematic batch, not a deficit calculation. Existing nearby actors can also participate.
	const int32 ExistingCount = BossSacrificeEnemies.Num();
	const UReEchoRunSubsystem* Run =
	    GetGameInstance() ? GetGameInstance()->GetSubsystem<UReEchoRunSubsystem>() : nullptr;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = Run ? Run->GetRunDataSnapshot() : nullptr;
	const FReEchoCsvSpawnPolicyRow* Policy = Snapshot ? Snapshot->FindEnabledSpawnPolicy() : nullptr;
	const FReEchoCsvEncounterRow* Encounter = Snapshot ? Snapshot->FindEncounter(CurrentEncounterId) : nullptr;
	FBox2D Bounds;
	int32 SpawnedCount = 0;
	if (Snapshot && Config->SacrificeSummonTypes.Num() > 0 && Policy && Encounter && EnemyRoster && Player &&
	    ArenaScene && ArenaScene->GetEnemySpawnWorldBounds(Bounds))
	{
		const int32 Needed = FMath::Clamp(Config->SacrificeSummonCount, 0, 20);
		FRandomStream Random(FMath::Rand());
		const float CrowdRotation = bOpeningCrowd ? Random.FRandRange(0.0f, 2.0f * PI) : 0.0f;
		int32 SectorCounts[OpeningCrowdSectors] = {};
		// Bounded random candidates: never hang a cinematic if the arena is full or obstructed.
		for (int32 Attempt = 0; Attempt < 640 && SpawnedCount < Needed; ++Attempt)
		{
			const int32 Sector = Attempt % OpeningCrowdSectors;
			if (bOpeningCrowd && SectorCounts[Sector] >= OpeningCrowdSectorQuota(Needed, Sector))
			{
				continue;
			}
			const FName EnemyId = Config->SacrificeSummonTypes[SpawnedCount % Config->SacrificeSummonTypes.Num()];
			const FReEchoCsvEnemyRow* Row = Snapshot->FindEnemy(EnemyId);
			if (!Row || !Row->bEnabled || Row->Archetype == TEXT("Boss"))
			{
				break;
			}
			const float Radius = FMath::Max(1.0f, Row->CollisionRadiusCm);
			const float HalfHeight = FMath::Max(Radius, Row->CollisionHalfHeightCm);
			const float Spacing = Radius * 2.0f + 30.0f;
			const float Distance = Config->SacrificeRadiusCm * FMath::Sqrt(Random.FRandRange(0.09f, 1.0f));
			const float Angle = Random.FRandRange(0.0f, 2.0f * PI);
			FVector Location =
			    Boss->GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
			if (bOpeningCrowd)
			{
				Location = Boss->GetActorLocation() +
				           SampleOpeningCrowdOffset(Random, Sector, CrowdRotation, Config->SacrificeRadiusCm);
			}
			Location.Z = ArenaScene->GetGameplayPlaneWorldZ() + HalfHeight;
			const FVector ToCamera = Location - Camera->GetComponentLocation();
			const float HalfWidth = Camera->OrthoWidth * 0.5f;
			if (Location.X - Radius < Bounds.Min.X || Location.X + Radius > Bounds.Max.X ||
			    Location.Y - Radius < Bounds.Min.Y || Location.Y + Radius > Bounds.Max.Y ||
			    FVector::Dist2D(Location, Player->GetActorLocation()) <
			        FMath::Max(Policy->MinPlayerDistanceCm, Spacing) ||
			    (!bOpeningCrowd &&
			     (FVector::DotProduct(ToCamera, Camera->GetForwardVector()) <= 0.0f ||
			      FMath::Abs(FVector::DotProduct(ToCamera, Camera->GetRightVector())) + Radius > HalfWidth ||
			      FMath::Abs(FVector::DotProduct(ToCamera, Camera->GetUpVector())) + HalfHeight >
			          HalfWidth / FMath::Max(0.1f, Camera->AspectRatio))))
			{
				continue;
			}
			bool bOccupied = false;
			for (TActorIterator<AReEchoEnemyActor> It(GetWorld()); It && !bOccupied; ++It)
			{
				bOccupied = It->IsAlive() && FVector::Dist2D(Location, It->GetActorLocation()) < Spacing;
			}
			for (TActorIterator<AReEchoEchoActor> It(GetWorld()); It && !bOccupied; ++It)
			{
				bOccupied =
				    FVector::Dist2D(Location, It->GetActorLocation()) < FMath::Max(Policy->MinEchoDistanceCm, Spacing);
			}
			for (const FVector& Reserved : EncounterSpawnLocations)
			{
				bOccupied |= FVector::Dist2D(Location, Reserved) < Spacing;
			}
			if (bOccupied ||
			    GetWorld()->OverlapBlockingTestByChannel(
			        Location,
			        FQuat::Identity,
			        ECC_Pawn,
			        FCollisionShape::MakeBox(FVector(Radius, Radius, FMath::Max(1.0f, HalfHeight - 2.0f)))))
			{
				continue;
			}
			AReEchoEnemyActor* Enemy = nullptr;
			if (!SpawnConfiguredEnemy(EnemyId, Location, Run->EncounterIndex, &Enemy))
			{
				break;
			}
			++SpawnedCount;
			++SectorCounts[Sector];
			FreezeBossTransformationActors();
			UReEchoEnemyPresentationComponent* Visual =
			    Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>();
			if (Visual)
			{
				Visual->NormalizeSacrificeBornDuration();
				BossSacrificeEnemies.Add(Enemy);
			}
		}
	}
	UE_LOG(LogTemp,
	       Display,
	       TEXT("[BossSacrifice] existing=%d spawned=%d selected=%d; spawned enemies remain in combat"),
	       ExistingCount,
	       SpawnedCount,
	       BossSacrificeEnemies.Num());
}

void AReEchoGameMode::BeginBossSacrifice()
{
	bBossSacrificeStarted = true;
	for (const TWeakObjectPtr<AReEchoEnemyActor>& Weak : BossSacrificeEnemies)
	{
		if (AReEchoEnemyActor* Enemy = Weak.Get())
		{
			UReEchoEnemyPresentationComponent* Visual =
			    Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>();
			if (Visual)
			{
				Visual->BeginSacrificeVisual();
			}
		}
	}
}

void AReEchoGameMode::UpdateBossSacrifice()
{
	if (!bBossSacrificeEnabled || BossTransformElapsed < BossTransformSettings.CameraPushSeconds)
	{
		return;
	}
	if (!bBossSacrificeStarted)
	{
		BeginBossSacrifice();
	}
	// Retain existing timing fields/DA serialization, but keep enemies visible throughout hover and fall.
	const bool bFalling = BossTransformElapsed >= BossSacrificeReappearStart;
	const bool bStartCharge = !bBossSacrificeChargeStarted &&
	                          BossTransformElapsed >= BossTransformSettings.CameraPushSeconds + 0.2f &&
	                          BossTransformElapsed < BossSacrificeReappearEnd;
	const float Rise = FMath::Clamp((BossTransformElapsed - BossSacrificeRiseStart) /
	                                    FMath::Max(0.01f, BossSacrificeRiseEnd - BossSacrificeRiseStart),
	                                0.0f,
	                                1.0f);
	const float Fall = FMath::Clamp((BossTransformElapsed - BossSacrificeReappearStart) /
	                                    FMath::Max(0.01f, BossSacrificeReappearEnd - BossSacrificeReappearStart),
	                                0.0f,
	                                1.0f);
	const float HeightAlpha = bFalling ? 1.0f - FMath::Square(Fall) : FMath::SmoothStep(0.0f, 1.0f, Rise);
	const UReEchoBossPhase3Config* Config =
	    BossPhase3Config ? BossPhase3Config.Get() : GetDefault<UReEchoBossPhase3Config>();
	const float HoldAlpha = FMath::Clamp((BossTransformElapsed - BossSacrificeRiseEnd) /
	                                         FMath::Max(0.01f, BossSacrificeReappearStart - BossSacrificeRiseEnd),
	                                     0.0f,
	                                     1.0f);
	// First 20% of the hold is motionless. Ramp shaking in/out without an endpoint jump.
	const float ShakeEnvelope =
	    bFalling ? 0.0f : FMath::SmoothStep(0.2f, 0.4f, HoldAlpha) * (1.0f - FMath::SmoothStep(0.85f, 1.0f, HoldAlpha));
	for (const TWeakObjectPtr<AReEchoEnemyActor>& Weak : BossSacrificeEnemies)
	{
		AReEchoEnemyActor* Enemy = Weak.Get();
		if (!Enemy)
		{
			continue;
		}
		UReEchoEnemyPresentationComponent* Visual = Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>();
		if (!Visual)
		{
			continue;
		}
		const float Phase = BossTransformElapsed * 2.0f * PI * 18.0f + Enemy->GetSpawnIndex() * 1.7f;
		if (BossTransformElapsed >= BossSacrificeRiseStart)
		{
			Visual->PlaySacrificeTransformAnimation();
		}
		const FVector Shake = FVector(FMath::Sin(Phase), FMath::Sin(Phase * 1.31f), 0.0f) *
		                      FMath::Max(0.0f, Config->SacrificeHoverShakeCm) * ShakeEnvelope;
		Visual->UpdateSacrificeVisual(HeightAlpha, 1.0f, Shake);
		if (Fall >= 1.0f && !bBossSacrificeReappearing)
		{
			// Restore cinematic offsets first, then commit the new form without replaying its transform.
			Visual->EndSacrificeVisual();
			if (!Enemy->CommitSacrificePhase2())
			{
				UE_LOG(LogTemp, Warning, TEXT("[BossSacrifice] phase2 commit rejected for %s"), *GetNameSafe(Enemy));
			}
			Visual->PrepareSacrificeGroundPose();
		}
		if (UReEchoCombatVfxComponent* Vfx = Enemy->FindComponentByClass<UReEchoCombatVfxComponent>())
		{
			if (bStartCharge)
			{
				Vfx->BeginSacrificeEffect(Visual->GetSacrificeVisualCenter());
			}
			if (Fall >= 1.0f)
			{
				Vfx->EndSacrificeEffect();
			}
			else
			{
				Vfx->UpdateSacrificeEffect(Visual->GetSacrificeVisualCenter());
			}
		}
	}
	bBossSacrificeChargeStarted |= bStartCharge;
	if (Fall >= 1.0f)
	{
		bBossSacrificeReappearing = true;
	}
}

void AReEchoGameMode::EndBossSacrifice()
{
	for (const TWeakObjectPtr<AReEchoEnemyActor>& Weak : BossSacrificeEnemies)
	{
		if (AReEchoEnemyActor* Enemy = Weak.Get())
		{
			if (UReEchoEnemyPresentationComponent* Visual =
			        Enemy->FindComponentByClass<UReEchoEnemyPresentationComponent>())
			{
				Visual->EndSacrificeVisual();
			}
			if (UReEchoCombatVfxComponent* Vfx = Enemy->FindComponentByClass<UReEchoCombatVfxComponent>())
			{
				Vfx->EndSacrificeEffect();
			}
		}
	}
	BossSacrificeEnemies.Reset();
	bBossSacrificeEnabled = false;
	bBossSacrificeStarted = false;
	bBossSacrificeChargeStarted = false;
	bBossSacrificeReappearing = false;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossOpeningCrowdDistributionTest,
                                 "ReEcho.Enemies.Host.Phase3OpeningCrowdDistribution",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossOpeningCrowdDistributionTest::RunTest(const FString& Parameters)
{
	for (int32 Total = 0; Total <= 20; ++Total)
	{
		int32 QuotaSum = 0;
		for (int32 Sector = 0; Sector < OpeningCrowdSectors; ++Sector)
		{
			QuotaSum += OpeningCrowdSectorQuota(Total, Sector);
		}
		TestEqual(TEXT("Sector quotas preserve configured total"), QuotaSum, Total);
	}
	FRandomStream Random(153);
	for (int32 Sector = 0; Sector < OpeningCrowdSectors; ++Sector)
	{
		TestTrue(TEXT("Twenty participants cover every direction with two or three slots"),
		         OpeningCrowdSectorQuota(20, Sector) >= 2 && OpeningCrowdSectorQuota(20, Sector) <= 3);
		for (int32 Sample = 0; Sample < 50; ++Sample)
		{
			const FVector Offset = SampleOpeningCrowdOffset(Random, Sector, 0.0f, 800.0f);
			float Angle = FMath::Atan2(Offset.Y, Offset.X);
			if (Angle < 0.0f)
			{
				Angle += 2.0f * PI;
			}
			TestTrue(TEXT("Random candidate stays inside its assigned sector"),
			         Angle >= Sector * 2.0f * PI / OpeningCrowdSectors - KINDA_SMALL_NUMBER &&
			             Angle <= (Sector + 1) * 2.0f * PI / OpeningCrowdSectors + KINDA_SMALL_NUMBER);
			TestTrue(TEXT("Candidates scatter inside the boss annulus, not on the center"),
			         Offset.Size2D() >= 240.0f - KINDA_SMALL_NUMBER && Offset.Size2D() <= 800.0f + KINDA_SMALL_NUMBER);
			TestEqual(TEXT("Ground height is assigned separately"), Offset.Z, 0.0);
		}
	}
	return true;
}
#endif
