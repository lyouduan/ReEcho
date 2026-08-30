#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Components/SceneComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraDataSetAccessor.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterInstance.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraVariant.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Data/ReEchoEnemyDefinitionCompiler.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Presentation/Combat/ReEchoCombatPresentationCoordinator.h"
#include "Presentation/Enemy/ReEchoEnemyPresentationComponent.h"
#include "Presentation/VFX/ReEchoVfxPreviewActor.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "Graybox/ReEchoProjectileActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr double FoxDirectionOriginalPivotY = 0.370447;
constexpr double FoxDirectionKuangAlphaBoundsCenterY = 0.4892367906066536;
constexpr double FoxDirectionKuang002AlphaBoundsCenterY = 0.5;

double ResolveFoxDirectionHalfCorrectionPivotY(const FName EmitterName)
{
	const double AlphaBoundsCenterY =
	    EmitterName == TEXT("Kuang") ? FoxDirectionKuangAlphaBoundsCenterY : FoxDirectionKuang002AlphaBoundsCenterY;
	return 0.5 * (FoxDirectionOriginalPivotY + AlphaBoundsCenterY);
}

struct FReEchoCombatVfxWorldFixture
{
	UWorld* World = nullptr;

	FReEchoCombatVfxWorldFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoCombatVfxTestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoCombatVfxWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossBlinkSlamPresentationTest,
                                 "ReEcho.Presentation.VFX.BossBlinkSlamPresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossBlinkSlamPresentationTest::RunTest(const FString& Parameters)
{
	const FVector StartOffset = UReEchoEnemyPresentationComponent::ResolveBlinkSlamVisualOffset(0.5f, 0.5f, 300.0f);
	const FVector HalfwayOffset = UReEchoEnemyPresentationComponent::ResolveBlinkSlamVisualOffset(0.25f, 0.5f, 300.0f);
	const FVector LandedOffset = UReEchoEnemyPresentationComponent::ResolveBlinkSlamVisualOffset(0.0f, 0.5f, 300.0f);

	TestTrue(TEXT("Blink slam starts 300 cm above the locked landing point"),
	         StartOffset.Equals(FVector(0.0f, 0.0f, 300.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Blink slam accelerates downward during its 0.5 second presentation"),
	         HalfwayOffset.Equals(FVector(0.0f, 0.0f, 75.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Blink slam presentation finishes exactly at the gameplay landing point"),
	         LandedOffset.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyDeathKnockbackPresentationTest,
                                 "ReEcho.Presentation.VFX.EnemyDeathKnockbackPresentation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyDeathKnockbackPresentationTest::RunTest(const FString& Parameters)
{
	const FVector Direction(3.0f, 4.0f, 0.0f);
	const FVector StartOffset =
	    UReEchoEnemyPresentationComponent::ResolveDeathKnockbackOffset(Direction, 0.0f, 0.3f, 90.0f);
	const FVector HalfwayOffset =
	    UReEchoEnemyPresentationComponent::ResolveDeathKnockbackOffset(Direction, 0.15f, 0.3f, 90.0f);
	const FVector EndOffset =
	    UReEchoEnemyPresentationComponent::ResolveDeathKnockbackOffset(Direction, 0.3f, 0.3f, 90.0f);

	TestTrue(TEXT("Fatal presentation starts without a position jump"), StartOffset.IsNearlyZero());
	TestTrue(TEXT("Fatal presentation quickly covers most of its displacement"),
	         HalfwayOffset.Equals(Direction.GetSafeNormal() * 78.75f, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Fatal presentation reaches the stronger 90 cm displacement"),
	         EndOffset.Equals(Direction.GetSafeNormal() * 90.0f, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoFatalEnemyHurtVfxPolicyTest,
                                 "ReEcho.Presentation.VFX.FatalEnemyHurtPolicy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoFatalEnemyHurtVfxPolicyTest::RunTest(const FString& Parameters)
{
	AReEchoEnemyActor* Enemy = GetMutableDefault<AReEchoEnemyActor>();
	AReEchoVfxScenarioTargetActor* NonEnemy = GetMutableDefault<AReEchoVfxScenarioTargetActor>();
	FReEchoDamageEvent Event;
	Event.AppliedDamage = 5.0f;
	Event.DamageSource = EReEchoDamageSource::Player;
	Event.bFatal = true;
	Event.Target = Enemy;
	TestTrue(TEXT("Fatal positive damage keeps enemy hurt feedback"),
	         UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(Event, Enemy));

	Event.AppliedDamage = 0.0f;
	TestFalse(TEXT("Fatal zero damage does not produce enemy hurt feedback"),
	          UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(Event, Enemy));

	Event.AppliedDamage = 5.0f;
	Event.Target = NonEnemy;
	TestFalse(TEXT("Fatal non-enemy damage keeps the existing hurt-feedback policy"),
	          UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(Event, NonEnemy));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoFoxDirectionRuntimeTest,
                                 "ReEcho.Presentation.VFX.FoxDirectionRuntime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoFoxDirectionRuntimeTest::RunTest(const FString& Parameters)
{
	FReEchoCombatVfxWorldFixture Fixture;
	APlayerController* PlayerController = Fixture.World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Runtime direction fixture creates a real PlayerController"), PlayerController))
	{
		return false;
	}
	PlayerController->SetControlRotation(FRotator(-60.0f, 25.0f, 0.0f));
	if (!PlayerController->PlayerCameraManager)
	{
		PlayerController->SpawnPlayerCameraManager();
	}
	APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(Fixture.World, 0);
	if (!TestNotNull(TEXT("Runtime direction fixture resolves its PlayerCameraManager"), Camera))
	{
		return false;
	}
	FMinimalViewInfo CameraView;
	CameraView.Rotation = FRotator(-60.0f, 25.0f, 0.0f);
	Camera->SetCameraCachePOV(CameraView);
	TestTrue(TEXT("PlayerCameraManager consumes the non-horizontal view target"),
	         FMath::Abs(Camera->GetCameraRotation().Pitch) > KINDA_SMALL_NUMBER);
	const FRotationMatrix ViewRotation(Camera->GetCameraRotation());
	const FVector ViewRight = ViewRotation.GetUnitAxis(EAxis::Y);
	const FVector ViewUp = ViewRotation.GetUnitAxis(EAxis::Z);
	const FReEchoCsvLoadResult LoadResult =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production enemy CSV loads for Fox Direction"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	FReEchoEnemyDefinition FoxDefinition;
	FString CompileError;
	if (!TestTrue(
	        TEXT("Production M_FOX definition compiles for Direction"),
	        ReEchoEnemyDefinitionCompiler::Compile(*LoadResult.Snapshot, TEXT("M_FOX"), FoxDefinition, CompileError)))
	{
		AddError(CompileError);
		return false;
	}
	constexpr double GameplayPlaneWorldZ = 725.0;
	AReEchoEnemyActor* Fox = Fixture.World->SpawnActor<AReEchoEnemyActor>();
	if (!TestNotNull(TEXT("Fox presentation host spawns"), Fox))
	{
		return false;
	}
	if (!Fox->HasActorBegunPlay())
	{
		Fox->DispatchBeginPlay();
	}
	Fox->ConfigureGameplayPlane(GameplayPlaneWorldZ);
	Fox->SetEnemyId(TEXT("M_FOX"));
	if (!TestTrue(TEXT("Fox presentation host accepts the production M_FOX definition"),
	              Fox->ConfigureFromDefinition(FoxDefinition, 117)))
	{
		return false;
	}
	USceneComponent* OwnerRoot = Fox->GetRootComponent();
	USceneComponent* AttackVfxRoot = Cast<USceneComponent>(Fox->GetDefaultSubobjectByName(TEXT("AttackVfxRoot")));
	UReEchoCombatPresentationCoordinator* Coordinator =
	    Fox->FindComponentByClass<UReEchoCombatPresentationCoordinator>();
	UReEchoCombatVfxComponent* Vfx = Fox->FindComponentByClass<UReEchoCombatVfxComponent>();
	if (!TestNotNull(TEXT("Fox presentation coordinator exists"), Coordinator) ||
	    !TestNotNull(TEXT("Fox VFX adapter exists"), Vfx) ||
	    !TestNotNull(TEXT("Production Fox Owner RootComponent exists"), OwnerRoot) ||
	    !TestNotNull(TEXT("Production Fox AttackVfxRoot exists"), AttackVfxRoot))
	{
		return false;
	}
	TestTrue(TEXT("Fox presentation host began play"), Fox->HasActorBegunPlay());
	TestTrue(TEXT("Fox coordinator has a VFX phase consumer"), Coordinator->OnActionPhase.IsBound());

	FReEchoEnemySpecialActionEvent Windup;
	Windup.AbilityId = TEXT("M_FOX_Dash");
	Windup.Type = EReEchoEnemySpecialActionEventType::WindupStarted;
	const FVector LockedDirections[] = {FVector::ForwardVector,
	                                    FVector::RightVector,
	                                    -FVector::ForwardVector,
	                                    FVector(0.6f, 0.8f, 0.0f).GetSafeNormal()};
	for (int32 DirectionIndex = 0; DirectionIndex < UE_ARRAY_COUNT(LockedDirections); ++DirectionIndex)
	{
		const FVector& LockedDirection = LockedDirections[DirectionIndex];
		Windup.LockedDirection = LockedDirection;
		Coordinator->ConsumeSpecialActionForTests(Windup);
		UNiagaraComponent* RuntimeDirection = Vfx->GetDirectionEffectForTests();
		if (!TestNotNull(TEXT("Each locked direction creates a live Direction component"), RuntimeDirection))
		{
			return false;
		}
		bool bHasRotationOverride = false;
		const float ActualDegrees =
		    RuntimeDirection->GetVariableFloat(TEXT("User.DirectionSpriteRotationDegrees"), bHasRotationOverride);
		const float ExpectedDegrees = FMath::RadiansToDegrees(FMath::Atan2(
		    FVector::DotProduct(LockedDirection, -ViewUp), FVector::DotProduct(LockedDirection, ViewRight)));
		AddInfo(FString::Printf(TEXT("Fox Direction screen rotation locked=%s viewRight=%s viewUp=%s "
		                             "actualDegrees=%.3f expectedDegrees=%.3f"),
		                        *LockedDirection.ToString(),
		                        *ViewRight.ToString(),
		                        *ViewUp.ToString(),
		                        ActualDegrees,
		                        ExpectedDegrees));
		TestTrue(TEXT("Fox Direction writes the exact SpriteRotation override before activation"),
		         bHasRotationOverride);
		TestTrue(TEXT("Fox Direction screen rotation follows the actual PlayerCameraManager basis"),
		         FMath::IsNearlyEqual(ActualDegrees, ExpectedDegrees, KINDA_SMALL_NUMBER));
		if (DirectionIndex + 1 < UE_ARRAY_COUNT(LockedDirections))
		{
			FReEchoEnemySpecialActionEvent Cancelled = Windup;
			Cancelled.Type = EReEchoEnemySpecialActionEventType::ActionCancelled;
			Coordinator->ConsumeSpecialActionForTests(Cancelled);
		}
	}
	UNiagaraComponent* Direction = Vfx->GetDirectionEffectForTests();
	UNiagaraComponent* Charging = Vfx->GetChargingEffectForTests();
	if (!TestNotNull(TEXT("Fox Windup creates a live Direction component"), Direction) ||
	    !TestNotNull(TEXT("Fox Windup keeps its live Charging component"), Charging))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("Production Fox center placement: planeZ=%.3f actorZ=%.3f ownerRootZ=%.3f "
	                             "attackRootZ=%.3f directionComponentZ=%.3f chargingComponentZ=%.3f"),
	                        GameplayPlaneWorldZ,
	                        Fox->GetActorLocation().Z,
	                        OwnerRoot->GetComponentLocation().Z,
	                        AttackVfxRoot->GetComponentLocation().Z,
	                        Direction->GetComponentLocation().Z,
	                        Charging->GetComponentLocation().Z));
	TestEqual(TEXT("Production M_FOX collision half-height is 130 cm"), FoxDefinition.CollisionHalfHeightCm, 130.0f);
	TestEqual(TEXT("Production Fox center is 855 cm"), Fox->GetActorLocation().Z, 855.0);
	TestEqual(TEXT("Production Fox Owner RootComponent follows the Actor center"),
	          OwnerRoot->GetComponentLocation().Z,
	          Fox->GetActorLocation().Z);
	TestEqual(TEXT("Production Fox AttackVfxRoot stays on the gameplay plane"),
	          AttackVfxRoot->GetComponentLocation().Z,
	          GameplayPlaneWorldZ);
	TestEqual(
	    TEXT("Fox Direction attaches directly to the Owner RootComponent"), Direction->GetAttachParent(), OwnerRoot);
	TestEqual(TEXT("Fox Direction component origin equals the Actor center"),
	          Direction->GetComponentLocation(),
	          Fox->GetActorLocation());
	TestEqual(
	    TEXT("Fox Direction placement remains zero-offset"), Direction->GetRelativeLocation(), FVector::ZeroVector);
	TestEqual(TEXT("Fox Charging remains attached to AttackVfxRoot"), Charging->GetAttachParent(), AttackVfxRoot);
	TestEqual(TEXT("Fox Charging component stays on the gameplay plane"),
	          Charging->GetComponentLocation().Z,
	          GameplayPlaneWorldZ);
	TestEqual(TEXT("Fox Charging placement remains zero-offset"), Charging->GetRelativeLocation(), FVector::ZeroVector);
	TestTrue(TEXT("Fox Direction component is registered"), Direction->IsRegistered());
	TestTrue(TEXT("Fox Direction component is visible"), Direction->IsVisible());
	TestTrue(TEXT("Fox Direction component is active"), Direction->IsActive());
	TestTrue(TEXT("Fox Direction component has a renderable world scale"),
	         Direction->GetComponentScale().GetAbsMin() > KINDA_SMALL_NUMBER);
	const FBox RuntimeBounds = Direction->GetSystemFixedBounds();
	TestTrue(TEXT("Fox Direction runtime override covers the authored sprite center"),
	         RuntimeBounds.IsInsideOrOn(FVector(0.0f, 0.0f, -150.0f)));
	TestTrue(TEXT("Fox Direction runtime override covers the maximum authored sprite extents"),
	         RuntimeBounds.IsInsideOrOn(FVector(500.0f, 500.0f, -650.0f)));
	TestEqual(TEXT("Fox Direction stays in the combat foreground sort band"),
	          Direction->TranslucencySortPriority,
	          UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(0));

	Direction->AdvanceSimulation(4, 1.0f / 60.0f);
	const FNiagaraSystemInstanceControllerConstPtr Controller = Direction->GetSystemInstanceController();
	const FNiagaraSystemInstance* SystemInstance =
	    Controller.IsValid() ? Controller->GetSystemInstance_Unsafe() : nullptr;
	if (!TestNotNull(TEXT("Fox Direction owns an initialized Niagara system instance"), SystemInstance))
	{
		return false;
	}
	int32 TotalParticles = 0;
	int32 PositionCheckedParticles = 0;
	for (const FNiagaraEmitterInstanceRef& Emitter : SystemInstance->GetEmitters())
	{
		TotalParticles += Emitter->GetNumParticles();
		const FNiagaraDataSet& ParticleData = Emitter->GetParticleData();
		const FNiagaraDataSetAccessor<FNiagaraPosition> PositionAccessor(ParticleData, TEXT("Position"));
		const FNiagaraDataSetReaderFloat<FNiagaraPosition> PositionReader = PositionAccessor.GetReader(ParticleData);
		for (int32 ParticleIndex = 0; ParticleIndex < Emitter->GetNumParticles() && PositionReader.IsValid();
		     ++ParticleIndex)
		{
			const FVector LocalParticlePosition(PositionReader.Get(ParticleIndex));
			const FVector WorldParticlePosition =
			    Direction->GetComponentTransform().TransformPosition(LocalParticlePosition);
			AddInfo(FString::Printf(TEXT("Fox Direction particle observation local=%s world=%s componentOrigin=%s"),
			                        *LocalParticlePosition.ToString(),
			                        *WorldParticlePosition.ToString(),
			                        *Direction->GetComponentLocation().ToString()));
			TestEqual(TEXT("Fox Direction authored particle Local Z is zero"), LocalParticlePosition.Z, 0.0);
			TestTrue(TEXT("Fox Direction particle starts at the Owner Root component origin"),
			         WorldParticlePosition.Equals(Direction->GetComponentLocation(), KINDA_SMALL_NUMBER));
			++PositionCheckedParticles;
		}
		AddInfo(FString::Printf(TEXT("Fox Direction emitter '%s': sim=%d state=%d particles=%d bounds=%s"),
		                        *Emitter->GetEmitterHandle().GetUniqueInstanceName(),
		                        static_cast<int32>(Emitter->GetSimTarget()),
		                        static_cast<int32>(Emitter->GetExecutionState()),
		                        Emitter->GetNumParticles(),
		                        *Emitter->GetBounds().ToString()));
		Emitter->GetParticleData().Dump(
		    0,
		    Emitter->GetNumParticles(),
		    FString::Printf(TEXT("FoxDirection.%s"), *Emitter->GetEmitterHandle().GetName().ToString()));
		const FVersionedNiagaraEmitterData* EmitterData = Emitter->GetEmitterHandle().GetEmitterData();
		if (!TestNotNull(TEXT("Fox Direction runtime emitter retains compiled renderer data"), EmitterData))
		{
			continue;
		}
		for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
		{
			if (!Renderer || !Renderer->GetIsEnabled())
			{
				continue;
			}
			AddInfo(FString::Printf(TEXT("Fox Direction renderer class='%s'"), *Renderer->GetClass()->GetName()));
			TArray<UMaterialInterface*> Materials;
			Renderer->GetUsedMaterials(&Emitter.Get(), Materials);
			TestTrue(TEXT("Fox Direction enabled renderer resolves at least one material"), Materials.Num() > 0);
			for (const UMaterialInterface* Material : Materials)
			{
				if (!TestNotNull(TEXT("Fox Direction renderer material is valid"), Material))
				{
					continue;
				}
				AddInfo(FString::Printf(TEXT("Fox Direction material '%s': blend=%d niagaraMeshUsage=%d"),
				                        *Material->GetPathName(),
				                        static_cast<int32>(Material->GetBlendMode()),
				                        Material->GetUsageByFlag(MATUSAGE_NiagaraMeshParticles) ? 1 : 0));
				if (Cast<UNiagaraSpriteRendererProperties>(Renderer))
				{
					TestTrue(TEXT("Fox Direction sprite material supports Niagara sprites"),
					         Material->GetUsageByFlag(MATUSAGE_NiagaraSprites));
				}
			}
			if (const UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
			{
				AddInfo(FString::Printf(TEXT("Fox Direction mesh renderer: facing=%d locked=%d axis=%s source=%d"),
				                        static_cast<int32>(MeshRenderer->FacingMode),
				                        MeshRenderer->bLockedAxisEnable ? 1 : 0,
				                        *MeshRenderer->LockedAxis.ToString(),
				                        static_cast<int32>(MeshRenderer->SourceMode)));
				for (const FNiagaraMeshRendererMeshProperties& MeshSlot : MeshRenderer->Meshes)
				{
					TestNotNull(TEXT("Fox Direction mesh renderer resolves a static mesh"), MeshSlot.Mesh.Get());
					AddInfo(FString::Printf(TEXT("Fox Direction mesh '%s': bounds=%s scale=%s rotation=%s pivot=%s"),
					                        MeshSlot.Mesh ? *MeshSlot.Mesh->GetPathName() : TEXT("None"),
					                        MeshSlot.Mesh ? *MeshSlot.Mesh->GetBounds().GetBox().ToString()
					                                      : TEXT("Invalid"),
					                        *MeshSlot.Scale.ToString(),
					                        *MeshSlot.Rotation.ToString(),
					                        *MeshSlot.PivotOffset.ToString()));
				}
			}
			else if (const UNiagaraSpriteRendererProperties* SpriteRenderer =
			             Cast<UNiagaraSpriteRendererProperties>(Renderer))
			{
				const FName EmitterName = Emitter->GetEmitterHandle().GetName();
				const double ExpectedPivotY = ResolveFoxDirectionHalfCorrectionPivotY(EmitterName);
				TestEqual(TEXT("Fox Direction SpriteRotation binds the exact exposed user parameter"),
				          SpriteRenderer->SpriteRotationBinding.GetParamMapBindableVariable().GetName(),
				          FName(TEXT("User.DirectionSpriteRotationDegrees")));
				TestFalse(TEXT("Fox Direction keeps its constant renderer pivot authoritative"),
				          SpriteRenderer->PivotOffsetBinding.DoesBindingExistOnSource());
				TestEqual(TEXT("Fox Direction preserves the authored forward anchor on sprite-local X"),
				          SpriteRenderer->PivotInUVSpace.X,
				          0.0);
				TestTrue(TEXT("Fox Direction renderer uses the alpha-derived half-correction pivot on Y"),
				         FMath::IsNearlyEqual(SpriteRenderer->PivotInUVSpace.Y, ExpectedPivotY, UE_KINDA_SMALL_NUMBER));
				AddInfo(FString::Printf(TEXT("Fox Direction sprite renderer: facing=%d alignment=%d source=%d "
				                             "cameraCull=%d min=%.1f max=%.1f visibility=%u pivot=(%.9f,%.9f)"),
				                        static_cast<int32>(SpriteRenderer->FacingMode),
				                        static_cast<int32>(SpriteRenderer->Alignment),
				                        static_cast<int32>(SpriteRenderer->SourceMode),
				                        SpriteRenderer->bEnableCameraDistanceCulling ? 1 : 0,
				                        SpriteRenderer->MinCameraDistance,
				                        SpriteRenderer->MaxCameraDistance,
				                        SpriteRenderer->RendererVisibility,
				                        SpriteRenderer->PivotInUVSpace.X,
				                        SpriteRenderer->PivotInUVSpace.Y));
			}
		}
	}
	TestTrue(TEXT("Fox Direction runtime simulation produces particles"), TotalParticles > 0);
	TestTrue(TEXT("Fox Direction runtime test reads back authored particle positions"), PositionCheckedParticles > 0);

	FReEchoEnemySpecialActionEvent Committed = Windup;
	Committed.Type = EReEchoEnemySpecialActionEventType::ActionCommitted;
	Fox->GetEnemyEventsComponent()->PublishSpecialAction(Committed);
	TestNull(TEXT("Fox Direction is removed when the dash commits"), Vfx->GetDirectionEffectForTests());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatVfxCatalogTest,
                                 "ReEcho.Presentation.VFX.Catalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatVfxCatalogTest::RunTest(const FString& Parameters)
{
	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	TestNotEqual(
	    TEXT("Ordered Enhance reactions use distinct entered-element Niagara"),
	    FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceGrass)),
	    FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceWater)));
	TestNotEqual(TEXT("Rabbit and Fox burn body variants are distinct"),
	             FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn,
	                                                                   TEXT("Enemy.Rabbit"))),
	             FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn,
	                                                                   TEXT("Enemy.Fox"))));
	TestTrue(TEXT("TimeGuard burn resolves the supplied Goat body variant"),
	         FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn,
	                                                               TEXT("Enemy.TimeGuard")))
	             .Contains(TEXT("NS_Element_Fire_Goat")));
	TestTrue(TEXT("TimeGuard water reactions resolve the supplied Goat body variant"),
	         FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Vaporize,
	                                                               TEXT("Enemy.TimeGuard")))
	             .Contains(TEXT("NS_Element_Water_Goat")));
	const FReEchoElementReactionVfxPlacement GrowthPlacement =
	    FReEchoElementReactionVfxCatalog::ResolvePlacement(EReEchoElementReactionVfxSemantic::Growth);
	const FReEchoElementReactionVfxPlacement EnhanceGrassPlacement =
	    FReEchoElementReactionVfxCatalog::ResolvePlacement(EReEchoElementReactionVfxSemantic::EnhanceGrass);
	const FReEchoElementReactionVfxPlacement EnhanceWaterPlacement =
	    FReEchoElementReactionVfxCatalog::ResolvePlacement(EReEchoElementReactionVfxSemantic::EnhanceWater);
	TestTrue(TEXT("Reaction effects preserve authored world size"), GrowthPlacement.bPreserveWorldSize);
	TestTrue(TEXT("Growth matches the target Flipbook size"), GrowthPlacement.bMatchTargetFlipbookSize);
	TestTrue(TEXT("Enhance Grass matches the target Flipbook size"),
	         EnhanceGrassPlacement.bMatchTargetFlipbookSize);
	TestTrue(TEXT("Enhance Water matches the target Flipbook size"),
	         EnhanceWaterPlacement.bMatchTargetFlipbookSize);
	TestTrue(TEXT("Burn visual lifetime follows the timed Burn status"),
	         UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_F_G")));
	TestFalse(TEXT("Growth is emitted only by its resolved reaction event"),
	          UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_L_G")));
	TestFalse(TEXT("Vaporize remains a target-bound one-shot"),
	          UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_F_W")));
	const FName DebugReactionNames[] = {
	    TEXT("Burn"), TEXT("Vaporize"), TEXT("Growth"), TEXT("Conduct"), TEXT("EnhanceGrass"), TEXT("EnhanceWater")};
	for (const FName DebugReactionName : DebugReactionNames)
	{
		uint8 SemanticValue = 0;
		TestTrue(FString::Printf(TEXT("GM reaction '%s' resolves directly to VFX"), *DebugReactionName.ToString()),
		         UReEchoCombatVfxComponent::TryResolveDebugElementReactionSemantic(DebugReactionName, SemanticValue));
	}
	uint8 InvalidSemanticValue = 0;
	TestFalse(TEXT("Unknown GM reaction does not spawn an unrelated VFX"),
	          UReEchoCombatVfxComponent::TryResolveDebugElementReactionSemantic(TEXT("Unknown"), InvalidSemanticValue));
	TestEqual(TEXT("Conduct delay zero preserves simultaneous compatibility"),
	          UReEchoCombatVfxComponent::ResolveConductLinkScheduledTime(4, 0.0f),
	          0.0f);
	TestTrue(TEXT("Conduct BFS schedule preserves authoritative index order"),
	         FMath::IsNearlyEqual(UReEchoCombatVfxComponent::ResolveConductLinkScheduledTime(3, 0.2f), 0.6f));
	const FVector StartWorld(1250.0f, -375.0f, 80.0f);
	const FVector EndWorld(1550.0f, 25.0f, 180.0f);
	FVector StartParameter = FVector::ZeroVector;
	FVector EndParameter = FVector::ZeroVector;
	UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(StartWorld, EndWorld, StartParameter, EndParameter);
	TestTrue(TEXT("World-space Conduct preserves a non-zero source origin"),
	         StartParameter.Equals(StartWorld, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Conduct converts its Beam End to the source-relative target displacement"),
	         EndParameter.Equals(EndWorld - StartWorld, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Conduct absolute start plus relative end reconstructs the target world position"),
	         (StartParameter + EndParameter).Equals(EndWorld, KINDA_SMALL_NUMBER));
	const FTransform ConnectionEffectTransform(FRotator(0.0f, 35.0f, 0.0f), FVector(900.0f, -120.0f, 45.0f));
	const float ConnectionBoundsPaddingCm = 200.0f;
	const FBox ConnectionLocalBounds = UReEchoCombatVfxComponent::ResolveConnectionLinkLocalBounds(
	    ConnectionEffectTransform, StartWorld, EndWorld, ConnectionBoundsPaddingCm);
	const FVector ConnectionLocalStart = ConnectionEffectTransform.InverseTransformPosition(StartWorld);
	const FVector ConnectionLocalEnd = ConnectionEffectTransform.InverseTransformPosition(EndWorld);
	TestTrue(TEXT("Echo connection runtime bounds contain the source in effect-local space"),
	         ConnectionLocalBounds.IsInsideOrOn(ConnectionLocalStart));
	TestTrue(TEXT("Echo connection runtime bounds contain the target in effect-local space"),
	         ConnectionLocalBounds.IsInsideOrOn(ConnectionLocalEnd));
	TestTrue(TEXT("Echo connection runtime bounds retain a finite ribbon safety margin"),
	         ConnectionLocalBounds.GetExtent().GetMin() >= ConnectionBoundsPaddingCm);
	UWorld* ConductAnchorWorld = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	AReEchoVfxScenarioTargetActor* ConductSource =
	    ConductAnchorWorld ? ConductAnchorWorld->SpawnActor<AReEchoVfxScenarioTargetActor>() : nullptr;
	AReEchoVfxScenarioTargetActor* ConductTarget =
	    ConductAnchorWorld ? ConductAnchorWorld->SpawnActor<AReEchoVfxScenarioTargetActor>() : nullptr;
	FReEchoDamageEvent ConnectionHurtEvent;
	ConnectionHurtEvent.Target = ConductTarget;
	ConnectionHurtEvent.AppliedDamage = 5.0f;
	ConnectionHurtEvent.DamageSource = EReEchoDamageSource::Path;
	ConnectionHurtEvent.bFatal = true;
	TestTrue(TEXT("Fatal connection-line damage still produces target hurt feedback"),
	         UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(ConnectionHurtEvent, ConductTarget));
	ConnectionHurtEvent.DamageSource = EReEchoDamageSource::Player;
	TestFalse(TEXT("Fatal non-enemy damage keeps the existing hurt-feedback policy"),
	          UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(ConnectionHurtEvent, ConductTarget));
	ConnectionHurtEvent.bFatal = false;
	ConnectionHurtEvent.AppliedDamage = 0.0f;
	TestFalse(TEXT("Blocked connection-line damage does not produce target hurt feedback"),
	          UReEchoCombatVfxComponent::ShouldPlayTargetHurtEffect(ConnectionHurtEvent, ConductTarget));
	const FVector BossHurtRootWorld(125.0f, -80.0f, 20.0f);
	const FVector BossFlipbookCenterWorld(170.0f, -40.0f, 180.0f);
	TestTrue(TEXT("Boss hurt effect is raised halfway from its authored root toward its rendered center"),
	         UReEchoCombatVfxComponent::ResolveBossHurtEffectLocation(BossHurtRootWorld, BossFlipbookCenterWorld)
	             .Equals(FVector(125.0f, -80.0f, 100.0f), KINDA_SMALL_NUMBER));
	FVector ConductAnchorStart = FVector::ZeroVector;
	FVector ConductAnchorEnd = FVector::ZeroVector;
	TestFalse(TEXT("Conduct is suppressed when either explicit Hurt VFX root is missing"),
	          UReEchoCombatVfxComponent::TryResolveConductLinkAnchors(
	              ConductSource, ConductTarget, ConductAnchorStart, ConductAnchorEnd));
	if (ConductSource && ConductTarget)
	{
		USceneComponent* SourceHurtRoot = NewObject<USceneComponent>(ConductSource);
		USceneComponent* TargetHurtRoot = NewObject<USceneComponent>(ConductTarget);
		SourceHurtRoot->RegisterComponent();
		TargetHurtRoot->RegisterComponent();
		SourceHurtRoot->SetWorldLocation(StartWorld);
		TargetHurtRoot->SetWorldLocation(EndWorld);
		UReEchoCombatVfxComponent* SourceVfx = NewObject<UReEchoCombatVfxComponent>(ConductSource);
		UReEchoCombatVfxComponent* TargetVfx = NewObject<UReEchoCombatVfxComponent>(ConductTarget);
		ConductSource->AddInstanceComponent(SourceVfx);
		ConductTarget->AddInstanceComponent(TargetVfx);
		SourceVfx->RegisterComponent();
		TargetVfx->RegisterComponent();
		SourceVfx->ConfigureAttachmentRoots(nullptr, SourceHurtRoot);
		TargetVfx->ConfigureAttachmentRoots(nullptr, TargetHurtRoot);
		TestTrue(TEXT("Conduct resolves only from both explicit Hurt VFX roots"),
		         UReEchoCombatVfxComponent::TryResolveConductLinkAnchors(
		             ConductSource, ConductTarget, ConductAnchorStart, ConductAnchorEnd));
		TestTrue(TEXT("Conduct starts at the source Hurt VFX root"),
		         ConductAnchorStart.Equals(StartWorld, KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Conduct ends at the target Hurt VFX root"),
		         ConductAnchorEnd.Equals(EndWorld, KINDA_SMALL_NUMBER));
	}
	if (ConductAnchorWorld)
	{
		ConductAnchorWorld->DestroyWorld(false);
	}
	FVector BeamStart = FVector::ZeroVector;
	FVector BeamEnd = FVector::ZeroVector;
	const FVector BeamWarningCenter(300.0f, -120.0f, 40.0f);
	const FVector BeamDirection(0.6f, 0.8f, 0.0f);
	const FVector BeamGroundOrigin = UReEchoCombatVfxComponent::ResolveBossBeamGroundOrigin(BeamWarningCenter, -25.0f);
	TestTrue(TEXT("Boss beam keeps the warning center XY and starts on the Arena ground plane"),
	         BeamGroundOrigin.Equals(FVector(300.0f, -120.0f, -25.0f), KINDA_SMALL_NUMBER));
	UReEchoCombatVfxComponent::ResolveBossBeamWorldEndpoints(
	    BeamGroundOrigin, BeamDirection, 750.0f, BeamStart, BeamEnd);
	TestTrue(TEXT("Boss beam starts at the grounded authoritative warning center"),
	         BeamStart.Equals(BeamGroundOrigin, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Boss beam endpoint extends upward from its ground origin"),
	         BeamEnd.Equals(BeamGroundOrigin + FVector::ForwardVector * 750.0f, KINDA_SMALL_NUMBER));
	const FVector BlinkWarningCenter(640.0f, -275.0f, 50.0f);
	const FVector BossLanding = AReEchoEnemyActor::ResolveBossLandingLocation(BlinkWarningCenter, 183.6f);
	TestTrue(TEXT("Blink Slam landing shares the warning center in arena XY"),
	         FVector::DistSquared2D(BossLanding, BlinkWarningCenter) <= KINDA_SMALL_NUMBER);
	TestEqual(TEXT("Blink Slam landing preserves the Boss gameplay height"), BossLanding.Z, 183.6);
	const FReEchoVfxPlacement GoatChargingPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::GoatSkill02Charging);
	TestTrue(TEXT("Goat body charging preserves authored world size"),
	         GoatChargingPlacement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize);
	TestTrue(TEXT("Preserved world-size VFX cancels inherited uniform owner scale"),
	         UReEchoCombatVfxComponent::ResolveAttachedScale(FVector::OneVector, FVector(2.0f), true)
	             .Equals(FVector(0.5f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Attack range scales only the configured local radial axis"),
	         UReEchoCombatVfxComponent::ResolveAttackRangeScale(
	             FVector(2.0f, 3.0f, 4.0f), FVector(0.0f, 1.0f, 0.0f), 1.5f, 0.5f, 2.0f)
	             .Equals(FVector(2.0f, 4.5f, 4.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Attack range multiplier is presentation-clamped without moving unmasked axes"),
	         UReEchoCombatVfxComponent::ResolveAttackRangeScale(
	             FVector::OneVector, FVector(1.0f, 1.0f, 0.0f), 3.0f, 0.8f, 2.0f)
	             .Equals(FVector(2.0f, 2.0f, 1.0f), KINDA_SMALL_NUMBER));
	const FReEchoVfxPlacement SwordRangePlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::PlayerMeleeSlash);
	TestTrue(TEXT("Sword slash opts into attack-range scaling"), SwordRangePlacement.bScaleWithAttackRange);
	TestTrue(TEXT("Sword slash scales only its local radial direction"),
	         SwordRangePlacement.AttackRangeScaleMask.Equals(FVector(0.0f, 1.0f, 0.0f), KINDA_SMALL_NUMBER));
	const FReEchoVfxPlacement ScytheRangePlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::PlayerScytheSlash);
	TestTrue(TEXT("Scythe slash opts into attack-range scaling"), ScytheRangePlacement.bScaleWithAttackRange);
	TestTrue(TEXT("Scythe slash scales in both local camera-plane axes"),
	         ScytheRangePlacement.AttackRangeScaleMask.Equals(FVector(1.0f, 1.0f, 0.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Scythe slash keeps the authored ten-percent size margin"),
	         ScytheRangePlacement.Scale.Equals(FVector(0.55f, 0.55f, 1.0f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Ordinary attached VFX retains its configured relative scale"),
	         UReEchoCombatVfxComponent::ResolveAttachedScale(FVector(1.2f, 0.8f, 1.0f), FVector(2.0f), false)
	             .Equals(FVector(1.2f, 0.8f, 1.0f), KINDA_SMALL_NUMBER));
	const FReEchoVfxPlacement SwordPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::PlayerMeleeSlash);
	TestTrue(TEXT("Sword slash uses the committed world attack direction"), SwordPlacement.bUseWorldDirectionRotation);
	TestEqual(TEXT("Forward longsword slash releases its VFX immediately"),
	          FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(EReEchoCombatVfxSemantic::PlayerMeleeSlash),
	          0.0f);
	TestEqual(TEXT("Scythe slash starts on the same committed frame as its sphere damage"),
	          FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(EReEchoCombatVfxSemantic::PlayerScytheSlash),
	          0.0f);
	TestEqual(TEXT("Element scythe slash also starts without presentation delay"),
	          FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(EReEchoCombatVfxSemantic::PlayerScytheSlashFlame),
	          0.0f);
	TestTrue(TEXT("Sword slash placement comes from its weapon profile"),
	         SwordPlacement.LocalOffset.Equals(FVector(0.0f, 0.0f, 60.0f), KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Sword slash consumes a finite artist-authored rotation"),
	          SwordPlacement.LocalRotation.ContainsNaN());
	TestFalse(TEXT("Sword slash consumes a finite in-plane Roll correction"),
	          FMath::IsNaN(SwordPlacement.LocalRotation.Roll));
	TestTrue(
	    TEXT("Sword slash cancels different host scales"),
	    UReEchoCombatVfxComponent::ResolveAttachedScale(
	        SwordPlacement.Scale, FVector(2.0f), SwordPlacement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize)
	        .Equals(SwordPlacement.Scale * 0.5f, KINDA_SMALL_NUMBER));
	const FVector SlashDirections[] = {FVector::ForwardVector, FVector::BackwardVector, FVector(0.6f, 0.8f, 0.0f)};
	TestEqual(TEXT("Left-side sword slash plays forward"),
	          UReEchoCombatVfxComponent::ResolveMeleePlayDirection(-FVector::RightVector, FVector::RightVector),
	          1.0f);
	TestEqual(TEXT("Right-side sword slash plays in reverse"),
	          UReEchoCombatVfxComponent::ResolveMeleePlayDirection(FVector::RightVector, FVector::RightVector),
	          -1.0f);
	for (const FVector& SlashDirection : SlashDirections)
	{
		const FRotator DirectionRotation =
		    UReEchoCombatVfxComponent::ResolveGroundPlaneDirectionRotation(SlashDirection);
		const FVector ExpectedPlaneDirection = FVector(SlashDirection.X, SlashDirection.Y, 0.0f).GetSafeNormal();
		TestTrue(
		    TEXT("Scythe VFX rotates in the ground plane toward the committed enemy"),
		    DirectionRotation.RotateVector(FVector::RightVector).Equals(ExpectedPlaneDirection, KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Scythe mesh local X surface normal remains world-up for a floor-parallel 360 sweep"),
		         DirectionRotation.RotateVector(FVector::ForwardVector).Equals(FVector::UpVector, KINDA_SMALL_NUMBER));
		const FRotator SwordDirectionRotation =
		    UReEchoCombatVfxComponent::ResolveSwordMeshDirectionRotation(SlashDirection);
		TestTrue(
		    TEXT("Sword slash keeps its authored local X surface normal world-up"),
		    SwordDirectionRotation.RotateVector(FVector::ForwardVector).Equals(FVector::UpVector, KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Sword slash maps its authored local Y attack axis to the committed ground direction"),
		         SwordDirectionRotation.RotateVector(FVector::RightVector)
		             .Equals(ExpectedPlaneDirection, KINDA_SMALL_NUMBER));
		const FRotator ComposedSwordRotation = UReEchoCombatVfxComponent::ComposeAttachedRotation(
		    SwordDirectionRotation, FRotator(0.0f, 0.0f, SwordPlacement.LocalRotation.Roll));
		TestTrue(
		    TEXT("Sword in-plane Roll correction preserves its world-up surface normal"),
		    ComposedSwordRotation.RotateVector(FVector::ForwardVector).Equals(FVector::UpVector, KINDA_SMALL_NUMBER));
	}
	const FVector MovedEndWorld(-240.0f, 910.0f, 25.0f);
	UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(
	    StartWorld, MovedEndWorld, StartParameter, EndParameter);
	TestTrue(TEXT("Moved target recomputes its source-relative Beam End rather than retaining the event snapshot"),
	         EndParameter.Equals(MovedEndWorld - StartWorld, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Moved target relative Beam End reconstructs the current target world position"),
	         (StartParameter + EndParameter).Equals(MovedEndWorld, KINDA_SMALL_NUMBER));
	const FString ConductPath =
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Conduct, TEXT("Enemy.Slime"));
	UNiagaraSystem* ConductSystem = LoadObject<UNiagaraSystem>(nullptr, *ConductPath);
	TestNotNull(TEXT("Conduct Niagara loads for coordinate-space contract validation"), ConductSystem);
	if (ConductSystem)
	{
		TArray<FNiagaraVariable> UserParameters;
		ConductSystem->GetExposedParameters().GetUserParameters(UserParameters);
		for (const FNiagaraVariable& Variable : UserParameters)
		{
			AddInfo(FString::Printf(TEXT("Conduct user parameter Name=%s Type=%s"),
			                        *Variable.GetName().ToString(),
			                        *Variable.GetType().GetName()));
		}
		const FNiagaraVariable* StartPositionParameter = UserParameters.FindByPredicate(
		    [](const FNiagaraVariable& Variable)
		    {
			    return Variable.GetName() == TEXT("StartPosition");
		    });
		const FNiagaraVariable* EndPositionParameter = UserParameters.FindByPredicate(
		    [](const FNiagaraVariable& Variable)
		    {
			    return Variable.GetName() == TEXT("EndPosition");
		    });
		TestTrue(TEXT("Conduct exposes exact User.StartPosition Niagara Position"),
		         StartPositionParameter &&
		             StartPositionParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef());
		TestTrue(TEXT("Conduct exposes exact User.EndPosition Niagara Position"),
		         EndPositionParameter && EndPositionParameter->GetType() == FNiagaraTypeDefinition::GetPositionDef());
		UNiagaraComponent* ParameterComponent = NewObject<UNiagaraComponent>();
		ParameterComponent->SetAsset(ConductSystem);
		ParameterComponent->SetAutoActivate(false);
		ParameterComponent->SetVariablePosition(TEXT("User.StartPosition"), StartWorld);
		ParameterComponent->SetVariablePosition(TEXT("User.EndPosition"), EndWorld);
		const FNiagaraVariant StartOverride = ParameterComponent->FindParameterOverride(
		    FNiagaraVariableBase(FNiagaraTypeDefinition::GetPositionDef(), TEXT("StartPosition")));
		const FNiagaraVariant EndOverride = ParameterComponent->FindParameterOverride(
		    FNiagaraVariableBase(FNiagaraTypeDefinition::GetPositionDef(), TEXT("EndPosition")));
		TestTrue(TEXT("Position API writes Start before activation"), StartOverride.IsValid());
		TestTrue(TEXT("Position API writes End before activation"), EndOverride.IsValid());
		TestEqual(TEXT("Start Position override stores an LWC FVector"),
		          StartOverride.GetNumBytes(),
		          static_cast<int32>(sizeof(FVector)));
		TestEqual(TEXT("End Position override stores an LWC FVector"),
		          EndOverride.GetNumBytes(),
		          static_cast<int32>(sizeof(FVector)));
		if (StartOverride.IsValid() && EndOverride.IsValid() && StartOverride.GetNumBytes() == sizeof(FVector) &&
		    EndOverride.GetNumBytes() == sizeof(FVector))
		{
			FVector StartReadback;
			FVector EndReadback;
			FMemory::Memcpy(&StartReadback, StartOverride.GetBytes(), sizeof(FVector));
			FMemory::Memcpy(&EndReadback, EndOverride.GetBytes(), sizeof(FVector));
			TestTrue(TEXT("Start Position override reads back the combat target world location"),
			         StartReadback.Equals(StartWorld, KINDA_SMALL_NUMBER));
			TestTrue(TEXT("End Position override reads back the combat target world location"),
			         EndReadback.Equals(EndWorld, KINDA_SMALL_NUMBER));
		}
		for (const FNiagaraEmitterHandle& Handle : ConductSystem->GetEmitterHandles())
		{
			const FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
			TestTrue(*FString::Printf(TEXT("Conduct emitter %s exposes coordinate-space data"),
			                          *Handle.GetName().ToString()),
			         EmitterData != nullptr);
			if (EmitterData)
			{
				TestFalse(*FString::Printf(TEXT("Conduct emitter %s uses world space"), *Handle.GetName().ToString()),
				          EmitterData->bLocalSpace);
			}
		}
	}
	UWorld* TimerWorld = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	AActor* TimerOwner = TimerWorld ? TimerWorld->SpawnActor<AActor>() : nullptr;
	UReEchoCombatVfxComponent* TimerComponent = TimerOwner ? NewObject<UReEchoCombatVfxComponent>(TimerOwner) : nullptr;
	if (TimerComponent)
	{
		TimerOwner->AddInstanceComponent(TimerComponent);
		TimerComponent->RegisterComponent();
		AReEchoVfxScenarioTargetActor* Target0 = TimerWorld->SpawnActor<AReEchoVfxScenarioTargetActor>();
		AReEchoVfxScenarioTargetActor* Target1 = TimerWorld->SpawnActor<AReEchoVfxScenarioTargetActor>();
		AReEchoVfxScenarioTargetActor* Target2 = TimerWorld->SpawnActor<AReEchoVfxScenarioTargetActor>();
		FReEchoElementReactionResolvedEvent TimedEvent;
		FReEchoElementReactionLink& FirstLink = TimedEvent.ReactionLinks.AddDefaulted_GetRef();
		FirstLink.SourceTarget = Target0;
		FirstLink.TargetTarget = Target1;
		FReEchoElementReactionLink& SecondLink = TimedEvent.ReactionLinks.AddDefaulted_GetRef();
		SecondLink.SourceTarget = Target1;
		SecondLink.TargetTarget = Target2;
		TimerComponent->ScheduleConductLinksForTests(TimedEvent, 0.2f);
		TestEqual(TEXT("Only delayed BFS links retain cancellable timers"),
		          TimerComponent->GetPendingConductTimerCountForTests(),
		          1);
		TimerComponent->ScheduleConductLinksForTests(TimedEvent, 0.2f);
		TestEqual(TEXT("A new reaction batch cancels instead of interleaving old links"),
		          TimerComponent->GetPendingConductTimerCountForTests(),
		          1);
		Target2->Destroy();
		TimerWorld->GetTimerManager().Tick(0.25f);
		TimerComponent->CancelConductPropagationForTests();
		TestEqual(TEXT("Explicit cleanup and EndPlay contract leave no pending Conduct timers"),
		          TimerComponent->GetPendingConductTimerCountForTests(),
		          0);
	}
	if (TimerWorld)
	{
		TimerWorld->DestroyWorld(false);
	}
	const FString PlayerHurt = FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerHurt);
	const FString EnemyHurt = FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EnemyHurt);
	TestTrue(TEXT("Player hurt uses the Rabbit-folder authority"), PlayerHurt.Contains(TEXT("/Monster/Rabbit/")));
	TestTrue(TEXT("Enemy hurt uses the Sword-folder authority"), EnemyHurt.Contains(TEXT("/People/Sword/")));
	TestNotEqual(TEXT("Same short name resolves to distinct packages"), PlayerHurt, EnemyHurt);
	TestTrue(TEXT("Long sword is melee"),
	         FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.LongSwordCombo")));
	TestTrue(TEXT("Scythe uses its dedicated melee Niagara"),
	         FReEchoCombatVfxCatalog::IsMeleeAttackPattern(TEXT("Pattern.ScytheSweep")));

	struct FLongSwordElementSlashCase
	{
		EReEchoElement Element;
		EReEchoCombatVfxSemantic Semantic;
		const TCHAR* ExpectedPath;
	};

	const FLongSwordElementSlashCase LongSwordElementSlashCases[] = {
	    {EReEchoElement::Flame,
	     EReEchoCombatVfxSemantic::PlayerMeleeSlashFlame,
	     TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Fire.NS_People_Sword_Attack_Fire")},
	    {EReEchoElement::Lightning,
	     EReEchoCombatVfxSemantic::PlayerMeleeSlashLightning,
	     TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Thunder.NS_People_Sword_Attack_Thunder")},
	    {EReEchoElement::Grass,
	     EReEchoCombatVfxSemantic::PlayerMeleeSlashGrass,
	     TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Grass.NS_People_Sword_Attack_Grass")},
	    {EReEchoElement::Water,
	     EReEchoCombatVfxSemantic::PlayerMeleeSlashWater,
	     TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Water.NS_People_Sword_Attack_Water")},
	};
	for (const FLongSwordElementSlashCase& SlashCase : LongSwordElementSlashCases)
	{
		EReEchoCombatVfxSemantic SlashSemantic = EReEchoCombatVfxSemantic::PlayerMeleeSlash;
		TestTrue(TEXT("Longsword element resolves a dedicated slash semantic"),
		         FReEchoCombatVfxCatalog::ResolveLongSwordSlashSemantic(SlashCase.Element, SlashSemantic));
		TestEqual(TEXT("Longsword element selects the expected slash semantic"), SlashSemantic, SlashCase.Semantic);
		TestTrue(TEXT("Resolved elemental slash remains in the Longsword semantic family"),
		         FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(SlashSemantic));
		TestEqual(TEXT("Longsword element selects the imported Niagara path"),
		          FReEchoCombatVfxCatalog::ResolvePath(SlashSemantic),
		          FString(SlashCase.ExpectedPath));
	}
	EReEchoCombatVfxSemantic NoneSlashSemantic = EReEchoCombatVfxSemantic::PlayerMeleeSlash;
	TestFalse(TEXT("Physical Longsword keeps its configured default slash"),
	          FReEchoCombatVfxCatalog::ResolveLongSwordSlashSemantic(EReEchoElement::None, NoneSlashSemantic));

	struct FScytheElementSlashCase
	{
		EReEchoElement Element;
		EReEchoCombatVfxSemantic Semantic;
		const TCHAR* ExpectedPath;
	};

	const FScytheElementSlashCase ScytheElementSlashCases[] = {
	    {EReEchoElement::Flame,
	     EReEchoCombatVfxSemantic::PlayerScytheSlashFlame,
	     TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Fire.NS_People_Sickle_Attack_Fire")},
	    {EReEchoElement::Lightning,
	     EReEchoCombatVfxSemantic::PlayerScytheSlashLightning,
	     TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Thunder.NS_People_Sickle_Attack_Thunder")},
	    {EReEchoElement::Grass,
	     EReEchoCombatVfxSemantic::PlayerScytheSlashGrass,
	     TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Grass.NS_People_Sickle_Attack_Grass")},
	    {EReEchoElement::Water,
	     EReEchoCombatVfxSemantic::PlayerScytheSlashWater,
	     TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Water.NS_People_Sickle_Attack_Water")},
	};
	for (const FScytheElementSlashCase& SlashCase : ScytheElementSlashCases)
	{
		EReEchoCombatVfxSemantic SlashSemantic = EReEchoCombatVfxSemantic::PlayerScytheSlash;
		TestTrue(TEXT("Scythe element resolves a dedicated slash semantic"),
		         FReEchoCombatVfxCatalog::ResolveScytheSlashSemantic(SlashCase.Element, SlashSemantic));
		TestEqual(TEXT("Scythe element selects the expected slash semantic"), SlashSemantic, SlashCase.Semantic);
		TestTrue(TEXT("Resolved elemental slash remains in the Scythe semantic family"),
		         FReEchoCombatVfxCatalog::IsScytheSlashSemantic(SlashSemantic));
		TestEqual(TEXT("Scythe element selects the imported Niagara path"),
		          FReEchoCombatVfxCatalog::ResolvePath(SlashSemantic),
		          FString(SlashCase.ExpectedPath));
	}
	EReEchoCombatVfxSemantic NoneScytheSemantic = EReEchoCombatVfxSemantic::PlayerScytheSlash;
	TestFalse(TEXT("Physical Scythe keeps its configured default slash"),
	          FReEchoCombatVfxCatalog::ResolveScytheSlashSemantic(EReEchoElement::None, NoneScytheSemantic));
	const FVector CameraRight = FVector::RightVector;
	TestEqual(TEXT("Gun muzzle discards upward aim while retaining the right side"),
	          UReEchoCombatVfxComponent::ResolveGunMuzzleHorizontalDirection(FVector(0.0f, 0.2f, 0.98f), CameraRight),
	          CameraRight);
	TestEqual(TEXT("Gun muzzle discards downward aim while retaining the left side"),
	          UReEchoCombatVfxComponent::ResolveGunMuzzleHorizontalDirection(FVector(0.0f, -0.2f, -0.98f), CameraRight),
	          -CameraRight);
	EReEchoCombatVfxSemantic CommittedSemantic = EReEchoCombatVfxSemantic::EnemyHurt;
	TestTrue(TEXT("Gun shot resolves an immediate weapon-local muzzle semantic"),
	         FReEchoCombatVfxCatalog::ResolveAttackCommittedSemantic(TEXT("Pattern.GunShot"), CommittedSemantic));
	TestEqual(TEXT("Gun shot uses the dedicated muzzle semantic"),
	          CommittedSemantic,
	          EReEchoCombatVfxSemantic::PlayerGunMuzzle);
	TestFalse(TEXT("Bow shot does not reuse the gun muzzle semantic"),
	          FReEchoCombatVfxCatalog::ResolveAttackCommittedSemantic(TEXT("Pattern.BowShot"), CommittedSemantic));
	EReEchoCombatVfxSemantic DamageSemantic = EReEchoCombatVfxSemantic::EnemyHurt;
	TestTrue(TEXT("Long sword successful hit resolves its DamageApplied semantic"),
	         FReEchoCombatVfxCatalog::ResolveWeaponDamageSemantic(TEXT("W_J_01"), DamageSemantic));
	TestEqual(TEXT("Long sword hit uses its dedicated impact semantic"),
	          DamageSemantic,
	          EReEchoCombatVfxSemantic::PlayerLongSwordImpact);
	TestTrue(TEXT("Scythe successful hit resolves its DamageApplied semantic"),
	         FReEchoCombatVfxCatalog::ResolveWeaponDamageSemantic(TEXT("W_J_04"), DamageSemantic));
	TestEqual(TEXT("Scythe hit uses its dedicated impact semantic"),
	          DamageSemantic,
	          EReEchoCombatVfxSemantic::PlayerScytheImpact);
	TestFalse(TEXT("Non-melee weapon cannot enter the melee hit VFX path"),
	          FReEchoCombatVfxCatalog::ResolveWeaponDamageSemantic(TEXT("W_J_08"), DamageSemantic));
	TestTrue(TEXT("Bow no longer resolves a legacy projectile texture"),
	         AReEchoProjectileActor::ResolveWeaponTexturePath(TEXT("Bow")).IsEmpty());
	TestTrue(TEXT("Gun no longer resolves a legacy projectile texture"),
	         AReEchoProjectileActor::ResolveWeaponTexturePath(TEXT("Gun")).IsEmpty());
	TestTrue(TEXT("Sage MoonStaff helper keeps the existing light-wave art contract"),
	         AReEchoProjectileActor::ResolveWeaponTexturePath(TEXT("MoonStaff")).Contains(TEXT("StaffLightWave")));
	TestEqual(TEXT("Combat effects use the global foreground band above ordinary actors"),
	          UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(23),
	          1000);
	TestEqual(TEXT("Combat effects still render above an owner already beyond the foreground band"),
	          UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(1500),
	          1501);
	TestEqual(TEXT("Echo card auras render immediately below their owning character"),
	          UReEchoCombatVfxComponent::ResolveEchoAuraSortPriority(23),
	          22);
	TestNotEqual(TEXT("Water and Grass Echo auras use distinct systems"),
	             FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EchoWaterAura),
	             FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EchoGrassAura));
	FReEchoEnemyProjectileEvent BallZero;
	BallZero.Attack.Sequence = 17;
	BallZero.VolleyBallIndex = 0;
	FReEchoEnemyProjectileEvent BallOne = BallZero;
	BallOne.VolleyBallIndex = 1;
	const FReEchoProjectileVisualKey BallZeroKey = UReEchoCombatVfxComponent::ResolveProjectileVisualKey(BallZero);
	const FReEchoProjectileVisualKey BallOneKey = UReEchoCombatVfxComponent::ResolveProjectileVisualKey(BallOne);
	TestTrue(TEXT("Same committed volley and same ball resolve a stable visual key"),
	         BallZeroKey == UReEchoCombatVfxComponent::ResolveProjectileVisualKey(BallZero));
	TestFalse(TEXT("Two balls in one committed volley cannot overwrite the same visual"), BallZeroKey == BallOneKey);
	TestEqual(TEXT("Rabbit material core diameter matches the gameplay collider"),
	          UReEchoCombatVfxComponent::ResolveProjectileCoreDiameter(50.0f),
	          100.0f);
	TestEqual(TEXT("Rabbit additive glow is larger without changing collision"),
	          UReEchoCombatVfxComponent::ResolveProjectileGlowDiameter(50.0f),
	          150.0f);
	const FVector LockedPlayerDirection = FVector(0.6f, 0.8f, 0.0f).GetSafeNormal();
	const FRotator RabbitProjectileRotation =
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::RabbitProjectile, LockedPlayerDirection);
	const FVector RotatedThreeBallCenterAxis = RabbitProjectileRotation
	                                               .RotateVector(FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(
	                                                   EReEchoCombatVfxSemantic::RabbitProjectile))
	                                               .GetSafeNormal2D();
	TestTrue(TEXT("Rabbit three-ball authored center points at the locked player"),
	         RotatedThreeBallCenterAxis.Equals(LockedPlayerDirection, KINDA_SMALL_NUMBER));
	const FVector BowTargetDirection = FVector(-0.8f, 0.6f, 0.0f).GetSafeNormal();
	const FVector BowAuthoredForwardAxis =
	    FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(EReEchoCombatVfxSemantic::PlayerBowFlight);
	TestTrue(TEXT("Bow delivered Niagara arrowhead is authored along local positive Y"),
	         BowAuthoredForwardAxis.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	const FRotator BowFlightRotation =
	    FReEchoCombatVfxCatalog::ResolveRotation(EReEchoCombatVfxSemantic::PlayerBowFlight, BowTargetDirection);
	const FVector RotatedBowAuthoredAxis = BowFlightRotation.RotateVector(BowAuthoredForwardAxis).GetSafeNormal2D();
	TestTrue(TEXT("Bow authored arrow axis points from the shooter toward the target"),
	         RotatedBowAuthoredAxis.Equals(BowTargetDirection, KINDA_SMALL_NUMBER));

	struct FBowElementFlightCase
	{
		EReEchoElement Element;
		EReEchoCombatVfxSemantic Semantic;
		const TCHAR* ExpectedPath;
	};

	const FBowElementFlightCase BowElementFlightCases[] = {
	    {EReEchoElement::Flame,
	     EReEchoCombatVfxSemantic::PlayerBowFlightFlame,
	     TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Fire.NS_People_Bow_Attack_Fire")},
	    {EReEchoElement::Lightning,
	     EReEchoCombatVfxSemantic::PlayerBowFlightLightning,
	     TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Thunder.NS_People_Bow_Attack_Thunder")},
	    {EReEchoElement::Grass,
	     EReEchoCombatVfxSemantic::PlayerBowFlightGrass,
	     TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Grass.NS_People_Bow_Attack_Grass")},
	    {EReEchoElement::Water,
	     EReEchoCombatVfxSemantic::PlayerBowFlightWater,
	     TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Water.NS_People_Bow_Attack_Water")},
	};
	TSet<FString> BowElementFlightPaths;
	for (const FBowElementFlightCase& FlightCase : BowElementFlightCases)
	{
		EReEchoCombatVfxSemantic ResolvedSemantic = EReEchoCombatVfxSemantic::PlayerBowFlight;
		TestTrue(TEXT("Combat Bow element resolves a dedicated flight semantic"),
		         FReEchoCombatVfxCatalog::ResolveBowFlightSemantic(FlightCase.Element, ResolvedSemantic));
		TestEqual(
		    TEXT("Combat Bow element selects the expected flight semantic"), ResolvedSemantic, FlightCase.Semantic);
		const FString FlightPath = FReEchoCombatVfxCatalog::ResolvePath(ResolvedSemantic);
		TestEqual(
		    TEXT("Combat Bow element selects the imported Niagara path"), FlightPath, FString(FlightCase.ExpectedPath));
		TestFalse(TEXT("Each combat Bow element uses a distinct Niagara path"),
		          BowElementFlightPaths.Contains(FlightPath));
		BowElementFlightPaths.Add(FlightPath);
		TestTrue(TEXT("Element Bow uses the same authored positive Y flight axis"),
		         FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(ResolvedSemantic)
		             .Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
	}
	EReEchoCombatVfxSemantic NoneBowSemantic = EReEchoCombatVfxSemantic::PlayerBowFlight;
	TestFalse(TEXT("Non-element Bow projectile keeps the configured Travel fallback"),
	          FReEchoCombatVfxCatalog::ResolveBowFlightSemantic(EReEchoElement::None, NoneBowSemantic));
	const FReEchoVfxPlacement FoxDirectionPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::FoxDirection);
	TestTrue(TEXT("Fox windup arrow keeps a visible non-degenerate component scale"),
	         FoxDirectionPlacement.Scale.GetAbsMin() > KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Fox windup arrow uses the combat foreground sort band"),
	         UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(0) >= 1000);
	TestFalse(TEXT("Gun muzzle production slot resolves a configured Niagara path"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerGunMuzzle).IsEmpty());
	TestTrue(TEXT("Gun muzzle uses the delivered bullet spark"),
	         FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerGunMuzzle)
	             .Contains(TEXT("NS_People_Bullet_spark")));
	TestEqual(TEXT("Normal Bow and Gun impacts reuse the Gun bullet spark"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerProjectileImpact),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerGunMuzzle));
	TestNotEqual(TEXT("Explosion impact remains distinct from the normal projectile impact"),
	             FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerProjectileExplosionImpact),
	             FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerProjectileImpact));
	EReEchoCombatVfxSemantic ProjectileImpactSemantic = EReEchoCombatVfxSemantic::EnemyHurt;
	TestTrue(TEXT("Normal Bow resolves a ranged impact"),
	         FReEchoCombatVfxCatalog::ResolveProjectileImpactSemantic(TEXT("Bow"), 0.0f, ProjectileImpactSemantic));
	TestEqual(TEXT("Normal Bow reuses the shared projectile impact"),
	          ProjectileImpactSemantic,
	          EReEchoCombatVfxSemantic::PlayerProjectileImpact);
	TestTrue(TEXT("Explosive Gun resolves a ranged impact"),
	         FReEchoCombatVfxCatalog::ResolveProjectileImpactSemantic(TEXT("Gun"), 200.0f, ProjectileImpactSemantic));
	TestEqual(TEXT("A compiled explosion radius selects the shared explosion impact"),
	          ProjectileImpactSemantic,
	          EReEchoCombatVfxSemantic::PlayerProjectileExplosionImpact);
	TestFalse(
	    TEXT("Non-projectile weapon cannot resolve a ranged impact"),
	    FReEchoCombatVfxCatalog::ResolveProjectileImpactSemantic(TEXT("Scythe"), 300.0f, ProjectileImpactSemantic));

	struct FGunElementFlightCase
	{
		EReEchoElement Element;
		EReEchoCombatVfxSemantic Semantic;
		const TCHAR* ExpectedPath;
	};

	const FGunElementFlightCase GunElementFlightCases[] = {
	    {EReEchoElement::Flame,
	     EReEchoCombatVfxSemantic::PlayerGunFlightFlame,
	     TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Fire_Fly.NS_People_Bullet_Fire_Fly")},
	    {EReEchoElement::Lightning,
	     EReEchoCombatVfxSemantic::PlayerGunFlightLightning,
	     TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Thunder_Fly.NS_People_Bullet_Thunder_Fly")},
	    {EReEchoElement::Grass,
	     EReEchoCombatVfxSemantic::PlayerGunFlightGrass,
	     TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Grass_Fly.NS_People_Bullet_Grass_Fly")},
	    {EReEchoElement::Water,
	     EReEchoCombatVfxSemantic::PlayerGunFlightWater,
	     TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Water_Fly.NS_People_Bullet_Water_Fly")},
	};
	TSet<FString> GunElementFlightPaths;
	for (const FGunElementFlightCase& FlightCase : GunElementFlightCases)
	{
		EReEchoCombatVfxSemantic ResolvedSemantic = EReEchoCombatVfxSemantic::PlayerGunFlight;
		TestTrue(TEXT("Combat Gun element resolves a dedicated flight semantic"),
		         FReEchoCombatVfxCatalog::ResolveGunFlightSemantic(FlightCase.Element, ResolvedSemantic));
		TestEqual(
		    TEXT("Combat Gun element selects the expected flight semantic"), ResolvedSemantic, FlightCase.Semantic);
		const FString FlightPath = FReEchoCombatVfxCatalog::ResolvePath(ResolvedSemantic);
		TestEqual(
		    TEXT("Combat Gun element selects the imported Niagara path"), FlightPath, FString(FlightCase.ExpectedPath));
		TestFalse(TEXT("Each combat Gun element uses a distinct Niagara path"),
		          GunElementFlightPaths.Contains(FlightPath));
		GunElementFlightPaths.Add(FlightPath);
	}
	EReEchoCombatVfxSemantic NoneSemantic = EReEchoCombatVfxSemantic::PlayerGunFlight;
	TestFalse(TEXT("Non-element Gun projectile keeps the legacy Travel fallback"),
	          FReEchoCombatVfxCatalog::ResolveGunFlightSemantic(EReEchoElement::None, NoneSemantic));

	const EReEchoCombatVfxSemantic RequiredSystems[] = {
	    EReEchoCombatVfxSemantic::RabbitCharging,
	    EReEchoCombatVfxSemantic::RabbitProjectile,
	    EReEchoCombatVfxSemantic::PlayerHurt,
	    EReEchoCombatVfxSemantic::FoxCharging,
	    EReEchoCombatVfxSemantic::FoxDirection,
	    EReEchoCombatVfxSemantic::FoxDash,
	    EReEchoCombatVfxSemantic::FoxImpact,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlash,
	    EReEchoCombatVfxSemantic::PlayerScytheSlash,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlashFlame,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlashLightning,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlashGrass,
	    EReEchoCombatVfxSemantic::PlayerMeleeSlashWater,
	    EReEchoCombatVfxSemantic::PlayerScytheSlashFlame,
	    EReEchoCombatVfxSemantic::PlayerScytheSlashLightning,
	    EReEchoCombatVfxSemantic::PlayerScytheSlashGrass,
	    EReEchoCombatVfxSemantic::PlayerScytheSlashWater,
	    EReEchoCombatVfxSemantic::PlayerLongSwordImpact,
	    EReEchoCombatVfxSemantic::PlayerScytheImpact,
	    EReEchoCombatVfxSemantic::PlayerBowFlight,
	    EReEchoCombatVfxSemantic::PlayerBowFlightFlame,
	    EReEchoCombatVfxSemantic::PlayerBowFlightLightning,
	    EReEchoCombatVfxSemantic::PlayerBowFlightGrass,
	    EReEchoCombatVfxSemantic::PlayerBowFlightWater,
	    EReEchoCombatVfxSemantic::PlayerBowImpact,
	    EReEchoCombatVfxSemantic::PlayerGunFlight,
	    EReEchoCombatVfxSemantic::PlayerGunFlightFlame,
	    EReEchoCombatVfxSemantic::PlayerGunFlightLightning,
	    EReEchoCombatVfxSemantic::PlayerGunFlightGrass,
	    EReEchoCombatVfxSemantic::PlayerGunFlightWater,
	    EReEchoCombatVfxSemantic::PlayerGunMuzzle,
	    EReEchoCombatVfxSemantic::PlayerProjectileImpact,
	    EReEchoCombatVfxSemantic::PlayerProjectileExplosionImpact,
	    EReEchoCombatVfxSemantic::EnemyHurt,
	    EReEchoCombatVfxSemantic::EchoWaterAura,
	    EReEchoCombatVfxSemantic::EchoGrassAura,
	    EReEchoCombatVfxSemantic::EchoBorn,
	    EReEchoCombatVfxSemantic::EchoConnectionLine,
	    EReEchoCombatVfxSemantic::GoatSkill02Charging,
	    EReEchoCombatVfxSemantic::GoatSkill02Bullet,
	    EReEchoCombatVfxSemantic::GoatSkill02Impact,
	    EReEchoCombatVfxSemantic::GoatSkill03Charging,
	    EReEchoCombatVfxSemantic::GoatSkill03Alarming,
	    EReEchoCombatVfxSemantic::GoatSkill03Impact,
	    EReEchoCombatVfxSemantic::GoatSkill04Charging,
	    EReEchoCombatVfxSemantic::GoatSkill04Lighting,
	};
	TestEqual(TEXT("Sheep projectile flight uses the authored Skill02 bullet"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::GoatSkill02Bullet),
	          FString(TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet.NS_Goat_Skill02_Bullet")));
	TestEqual(TEXT("Sheep blink telegraph uses the authored Skill03 alarming system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::GoatSkill03Alarming),
	          FString(TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming.NS_Goat_Skill03_Alarming")));
	TestEqual(TEXT("Sheep beam active window uses the authored Skill04 lighting system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::GoatSkill04Lighting),
	          FString(TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting.NS_Goat_Skill04_Lighting")));
	TestEqual(TEXT("Fox windup uses the authored charging system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxCharging),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Charging.NS_Fox_Rush_Charging")));
	TestEqual(TEXT("Fox windup direction uses the authored arrow system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxDirection),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow.NS_Fox_Rush_arrow")));
	TestEqual(TEXT("Fox committed dash uses the authored rush system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxDash),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail.NS_Fox_Rush_Trail")));
	TestEqual(TEXT("Fox applied hit uses the authored impact system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::FoxImpact),
	          FString(TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_BeAttacked.NS_Fox_Rush_BeAttacked")));
	TestEqual(TEXT("Connection card uses the delivered Echo chain system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EchoConnectionLine),
	          FString(TEXT("/Game/VFX/Echo/Particle/NS_Echo_Chain.NS_Echo_Chain")));
	TestEqual(TEXT("Echo initialization uses the independent birth-circle system"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::EchoBorn),
	          FString(TEXT("/Game/VFX/Echo/Particle/NS_Echo_Born.NS_Echo_Born")));
	const FReEchoVfxPlacement EchoBornPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::EchoBorn);
	TestTrue(TEXT("Echo Born keeps a stable world size"),
	         EchoBornPlacement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize);
	TestTrue(TEXT("Echo Born renders at one quarter of the source circle size"),
	         EchoBornPlacement.Scale.Equals(FVector(0.25f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Echo Born owns a finite one-shot presentation lifetime"),
	         FMath::IsNearlyEqual(EchoBornPlacement.PlaybackDurationSeconds, 0.8f));
	TestTrue(TEXT("Echo Born scale resolves a requested world diameter from authored ground bounds"),
	         UReEchoCombatVfxComponent::ResolveEchoBornWorldScale(
	             120.0f, FBox(FVector(-500.0f, -300.0f, -100.0f), FVector(500.0f, 300.0f, 400.0f)), FVector(0.25f))
	             .Equals(FVector(0.2f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Echo Born invalid bounds retain the catalog fallback scale"),
	         UReEchoCombatVfxComponent::ResolveEchoBornWorldScale(120.0f, FBox(EForceInit::ForceInit), FVector(0.25f))
	             .Equals(FVector(0.25f), KINDA_SMALL_NUMBER));
	const FVector RequestedScreenDown(0.25f, -0.75f, -0.5f);
	const FVector ExpectedGroundArrow = FVector(RequestedScreenDown.X, RequestedScreenDown.Y, 0.0f).GetSafeNormal();
	const FQuat EchoBornRotation = UReEchoCombatVfxComponent::ResolveEchoBornWorldRotation(RequestedScreenDown);
	TestTrue(TEXT("Echo Born maps its local X normal to world up"),
	         EchoBornRotation.RotateVector(FVector::ForwardVector).Equals(FVector::UpVector, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Echo Born maps its local Z arrow to screen down on the ground plane"),
	         EchoBornRotation.RotateVector(FVector::UpVector).Equals(ExpectedGroundArrow, KINDA_SMALL_NUMBER));
	const FBox OffsetBounds(FVector(-20.0f, -200.0f, -50.0f), FVector(40.0f, 400.0f, 250.0f));
	const FVector RequestedCenter(700.0f, -300.0f, 25.0f);
	const FVector TestScale(0.5f);
	const FVector ResolvedComponentLocation = UReEchoCombatVfxComponent::ResolveEchoBornComponentLocation(
	    RequestedCenter, OffsetBounds, TestScale, EchoBornRotation);
	const FVector LocalPlaneCenter(0.0f, OffsetBounds.GetCenter().Y, OffsetBounds.GetCenter().Z);
	TestTrue(TEXT("Echo Born authored YZ center resolves exactly onto the Flipbook bottom center"),
	         (ResolvedComponentLocation + EchoBornRotation.RotateVector(LocalPlaneCenter * TestScale))
	             .Equals(RequestedCenter, KINDA_SMALL_NUMBER));
	for (const EReEchoCombatVfxSemantic Semantic : RequiredSystems)
	{
		const FString AssetPath = FReEchoCombatVfxCatalog::ResolvePath(Semantic);
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
		if (!TestNotNull(FString::Printf(TEXT("Niagara system loads: %s"), *AssetPath), System))
		{
			continue;
		}
		if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash)
		{
			TestFalse(TEXT("Replacement sword asset selects the explicit missing-parameter reverse fallback"),
			          UReEchoCombatVfxComponent::HasMeleePlayDirectionParameter(System));
		}
		if (Semantic == EReEchoCombatVfxSemantic::EchoConnectionLine)
		{
			TArray<FNiagaraVariable> UserParameters;
			System->GetExposedParameters().GetUserParameters(UserParameters);
			for (const FName RequiredParameter : {FName(TEXT("StartPosition")), FName(TEXT("EndPosition"))})
			{
				const FNiagaraVariable* Parameter = UserParameters.FindByPredicate(
				    [RequiredParameter](const FNiagaraVariable& Variable)
				    {
					    return Variable.GetName() == RequiredParameter;
				    });
				TestTrue(
				    FString::Printf(TEXT("Echo chain exposes Niagara Position User.%s"), *RequiredParameter.ToString()),
				    Parameter && Parameter->GetType() == FNiagaraTypeDefinition::GetPositionDef());
			}
		}
		if (Semantic == EReEchoCombatVfxSemantic::EchoBorn)
		{
			TestFalse(TEXT("Echo Born loads without deferred Niagara compilation"),
			          System->HasOutstandingCompilationRequests(true));
			for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
			{
				if (EmitterHandle.GetIsEnabled())
				{
					const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
					TestTrue(TEXT("Echo Born enabled emitters preserve their authored component-space layout"),
					         EmitterData && EmitterData->bLocalSpace);
					if (EmitterData)
					{
						for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
						{
							const UNiagaraSpriteRendererProperties* Sprite =
							    Cast<UNiagaraSpriteRendererProperties>(Renderer);
							if (Sprite && Sprite->GetIsEnabled())
							{
								TestTrue(TEXT("Echo Born sprites face the authored ground normal"),
								         Sprite->FacingMode == ENiagaraSpriteFacingMode::CustomFacingVector);
								TestEqual(TEXT("Echo Born sprite facing binds User.GroundNormal"),
								          Sprite->SpriteFacingBinding.GetParamMapBindableVariable().GetName(),
								          FName(TEXT("User.GroundNormal")));
								TestTrue(TEXT("Echo Born sprites keep a fixed tangent on the ground plane"),
								         Sprite->Alignment == ENiagaraSpriteAlignment::CustomAlignment);
								TestEqual(TEXT("Echo Born sprite alignment binds User.GroundTangent"),
								          Sprite->SpriteAlignmentBinding.GetParamMapBindableVariable().GetName(),
								          FName(TEXT("User.GroundTangent")));
							}
						}
					}
				}
			}
		}
		const bool bRequiresComponentSpace =
		    Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashFlame ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashLightning ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashGrass ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashWater ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashFlame ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashLightning ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashGrass ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashWater ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightFlame ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightLightning ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightGrass ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightWater ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunFlight ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightFlame ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightLightning ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightGrass ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightWater ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunMuzzle ||
		    Semantic == EReEchoCombatVfxSemantic::FoxDirection || Semantic == EReEchoCombatVfxSemantic::FoxDash ||
		    Semantic == EReEchoCombatVfxSemantic::EchoWaterAura || Semantic == EReEchoCombatVfxSemantic::EchoGrassAura;
		const bool bRequiresWeaponLocalSpace = Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashFlame ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashLightning ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashGrass ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlashWater ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashFlame ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashLightning ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashGrass ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlashWater ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightFlame ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightLightning ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightGrass ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerBowFlightWater ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunFlight ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightFlame ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightLightning ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightGrass ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunFlightWater ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunMuzzle;
		if (bRequiresComponentSpace)
		{
			int32 BowSpriteRendererCount = 0;
			int32 SwordMeshRendererCount = 0;
			int32 SwordSpriteRendererCount = 0;
			int32 ScytheMeshRendererCount = 0;
			int32 ScytheSpriteRendererCount = 0;
			int32 FoxDirectionEnabledEmitterCount = 0;
			int32 FoxDirectionEnabledRendererCount = 0;
			int32 FoxDirectionEnabledSpriteRendererCount = 0;
			int32 GunMuzzleEnabledSpriteRendererCount = 0;
			for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
			{
				if (!EmitterHandle.GetIsEnabled())
				{
					continue;
				}
				const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
				if (Semantic == EReEchoCombatVfxSemantic::FoxDirection)
				{
					++FoxDirectionEnabledEmitterCount;
					TestTrue(FString::Printf(TEXT("Fox direction emitter '%s' exposes compiled data"),
					                         *EmitterHandle.GetName().ToString()),
					         EmitterData != nullptr);
					if (EmitterData)
					{
						TestTrue(FString::Printf(TEXT("Fox direction emitter '%s' follows component-space rotation"),
						                         *EmitterHandle.GetName().ToString()),
						         EmitterData->bLocalSpace);
						for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
						{
							if (Renderer && Renderer->GetIsEnabled())
							{
								++FoxDirectionEnabledRendererCount;
								if (const UNiagaraSpriteRendererProperties* Sprite =
								        Cast<UNiagaraSpriteRendererProperties>(Renderer))
								{
									++FoxDirectionEnabledSpriteRendererCount;
									const FName EmitterName = EmitterHandle.GetName();
									const double ExpectedPivotY = ResolveFoxDirectionHalfCorrectionPivotY(EmitterName);
									TestTrue(TEXT("Fox Direction SpriteRotation binding exists on its source"),
									         Sprite->SpriteRotationBinding.DoesBindingExistOnSource());
									TestEqual(TEXT("Fox Direction enabled sprites share one exact rotation parameter"),
									          Sprite->SpriteRotationBinding.GetParamMapBindableVariable().GetName(),
									          FName(TEXT("User.DirectionSpriteRotationDegrees")));
									TestFalse(
									    TEXT("Fox Direction enabled sprites do not override their constant pivot"),
									    Sprite->PivotOffsetBinding.DoesBindingExistOnSource());
									TestEqual(TEXT("Fox Direction enabled sprites retain the forward anchor on X"),
									          Sprite->PivotInUVSpace.X,
									          0.0);
									TestTrue(TEXT("Fox Direction enabled sprites retain their alpha-derived "
									              "half-correction Y"),
									         FMath::IsNearlyEqual(
									             Sprite->PivotInUVSpace.Y, ExpectedPivotY, UE_KINDA_SMALL_NUMBER));
								}
							}
						}
					}
				}
				if (Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight && EmitterData)
				{
					for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
					{
						if (const UNiagaraSpriteRendererProperties* Sprite =
						        Cast<UNiagaraSpriteRendererProperties>(Renderer))
						{
							++BowSpriteRendererCount;
							TestEqual(TEXT("Bow sprite renderer preserves its authored alignment"),
							          Sprite->Alignment,
							          ENiagaraSpriteAlignment::Automatic);
						}
					}
				}
				if (Semantic == EReEchoCombatVfxSemantic::PlayerGunMuzzle && EmitterData)
				{
					for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
					{
						const UNiagaraSpriteRendererProperties* Sprite =
						    Cast<UNiagaraSpriteRendererProperties>(Renderer);
						if (!Sprite || !Sprite->GetIsEnabled())
						{
							continue;
						}
						++GunMuzzleEnabledSpriteRendererCount;
						TestTrue(TEXT("Gun muzzle SpriteRotation binding exists on its source"),
						         Sprite->SpriteRotationBinding.DoesBindingExistOnSource());
						TestEqual(TEXT("Gun muzzle sprites share the screen-direction rotation parameter"),
						          Sprite->SpriteRotationBinding.GetParamMapBindableVariable().GetName(),
						          FName(TEXT("User.DirectionSpriteRotationDegrees")));
					}
				}
				if (FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic) && EmitterData)
				{
					for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
					{
						if (const UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
						{
							++SwordMeshRendererCount;
							TestEqual(TEXT("Sword mesh renderer preserves component-space direction"),
							          Mesh->FacingMode,
							          ENiagaraMeshFacingMode::Default);
						}
						if (const UNiagaraSpriteRendererProperties* Sprite =
						        Cast<UNiagaraSpriteRendererProperties>(Renderer))
						{
							++SwordSpriteRendererCount;
							TestEqual(TEXT("Sword sprite faces the runtime ground normal"),
							          Sprite->FacingMode,
							          ENiagaraSpriteFacingMode::CustomFacingVector);
							TestEqual(TEXT("Sword sprite uses the runtime ground tangent"),
							          Sprite->Alignment,
							          ENiagaraSpriteAlignment::CustomAlignment);
							TestEqual(TEXT("Sword sprite normal binding is stable"),
							          Sprite->SpriteFacingBinding.GetParamMapBindableVariable().GetName(),
							          FName(TEXT("User.GroundNormal")));
							TestEqual(TEXT("Sword sprite tangent binding is stable"),
							          Sprite->SpriteAlignmentBinding.GetParamMapBindableVariable().GetName(),
							          FName(TEXT("User.GroundTangent")));
						}
					}
				}
				if (FReEchoCombatVfxCatalog::IsScytheSlashSemantic(Semantic) && EmitterData)
				{
					for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
					{
						if (const UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer))
						{
							++ScytheMeshRendererCount;
							TestEqual(TEXT("Scythe mesh renderer follows component ground-plane rotation"),
							          Mesh->FacingMode,
							          ENiagaraMeshFacingMode::Default);
						}
						if (const UNiagaraSpriteRendererProperties* Sprite =
						        Cast<UNiagaraSpriteRendererProperties>(Renderer))
						{
							++ScytheSpriteRendererCount;
							TestEqual(TEXT("Scythe sprite faces the runtime ground normal"),
							          Sprite->FacingMode,
							          ENiagaraSpriteFacingMode::CustomFacingVector);
							TestEqual(TEXT("Scythe sprite uses the runtime ground tangent"),
							          Sprite->Alignment,
							          ENiagaraSpriteAlignment::CustomAlignment);
							TestEqual(TEXT("Scythe sprite normal binding is stable"),
							          Sprite->SpriteFacingBinding.GetParamMapBindableVariable().GetName(),
							          FName(TEXT("User.GroundNormal")));
							TestEqual(TEXT("Scythe sprite tangent binding is stable"),
							          Sprite->SpriteAlignmentBinding.GetParamMapBindableVariable().GetName(),
							          FName(TEXT("User.GroundTangent")));
						}
					}
				}
				if (bRequiresWeaponLocalSpace)
				{
					TestTrue(FString::Printf(TEXT("Weapon VFX '%s' emitter '%s' uses local space"),
					                         *System->GetPathName(),
					                         *EmitterHandle.GetName().ToString()),
					         EmitterData && EmitterData->bLocalSpace);
				}
			}
			if (Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight)
			{
				TestEqual(TEXT("Bow flight keeps its three authored sprite layers"), BowSpriteRendererCount, 3);
			}
			if (FReEchoCombatVfxCatalog::IsLongSwordSlashSemantic(Semantic))
			{
				TestTrue(TEXT("Sword slash retains at least one authored mesh renderer"), SwordMeshRendererCount > 0);
				TestTrue(TEXT("Sword slash retains at least one authored sprite renderer"),
				         SwordSpriteRendererCount > 0);
			}
			if (FReEchoCombatVfxCatalog::IsScytheSlashSemantic(Semantic))
			{
				TestTrue(TEXT("Scythe slash retains at least one authored mesh renderer"), ScytheMeshRendererCount > 0);
				TestTrue(TEXT("Scythe slash retains at least one authored sprite renderer"),
				         ScytheSpriteRendererCount > 0);
			}
			if (Semantic == EReEchoCombatVfxSemantic::PlayerGunMuzzle)
			{
				TestTrue(TEXT("Gun muzzle contains at least one enabled Sprite renderer"),
				         GunMuzzleEnabledSpriteRendererCount > 0);
				TArray<FNiagaraVariable> GunMuzzleUserParameters;
				System->GetExposedParameters().GetUserParameters(GunMuzzleUserParameters);
				const FNiagaraVariable* RotationParameter = GunMuzzleUserParameters.FindByPredicate(
				    [](const FNiagaraVariable& Variable)
				    {
					    return Variable.GetName() == TEXT("DirectionSpriteRotationDegrees");
				    });
				TestTrue(TEXT("Gun muzzle exposes float User.DirectionSpriteRotationDegrees"),
				         RotationParameter && RotationParameter->GetType() == FNiagaraTypeDefinition::GetFloatDef());
			}
			if (Semantic == EReEchoCombatVfxSemantic::FoxDirection)
			{
				TestTrue(TEXT("Fox direction contains at least one enabled emitter"),
				         FoxDirectionEnabledEmitterCount > 0);
				TestTrue(TEXT("Fox direction contains at least one enabled renderer"),
				         FoxDirectionEnabledRendererCount > 0);
				TestEqual(TEXT("Fox direction binds both enabled Sprite renderers"),
				          FoxDirectionEnabledSpriteRendererCount,
				          2);
				TArray<FNiagaraVariable> FoxDirectionUserParameters;
				System->GetExposedParameters().GetUserParameters(FoxDirectionUserParameters);
				const FNiagaraVariable* RotationParameter = FoxDirectionUserParameters.FindByPredicate(
				    [](const FNiagaraVariable& Variable)
				    {
					    return Variable.GetName() == TEXT("DirectionSpriteRotationDegrees");
				    });
				TestTrue(TEXT("Fox direction exposes exact float User.DirectionSpriteRotationDegrees"),
				         RotationParameter && RotationParameter->GetType() == FNiagaraTypeDefinition::GetFloatDef());
				const FBox FixedBounds = System->GetFixedBounds();
				TestTrue(TEXT("Fox direction opts into fixed bounds for deterministic camera culling"),
				         System->bFixedBounds != 0);
				TestTrue(TEXT("Fox direction fixed bounds are valid and non-degenerate"),
				         FixedBounds.IsValid != 0 && FixedBounds.GetSize().GetAbsMin() > KINDA_SMALL_NUMBER);
			}
		}
		if (Semantic == EReEchoCombatVfxSemantic::GoatSkill03Alarming ||
		    Semantic == EReEchoCombatVfxSemantic::GoatSkill03Impact)
		{
			int32 GroundSpriteRendererCount = 0;
			for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
			{
				const FVersionedNiagaraEmitterData* EmitterData =
				    EmitterHandle.GetIsEnabled() ? EmitterHandle.GetEmitterData() : nullptr;
				if (!EmitterData)
				{
					continue;
				}
				TestTrue(TEXT("Scythe slash emitter positions follow the ground-aligned component transform"),
				         EmitterData->bLocalSpace);
				for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
				{
					const UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer);
					if (!Sprite)
					{
						continue;
					}
					++GroundSpriteRendererCount;
					TestEqual(TEXT("Goat ground effect uses a custom world-up facing vector"),
					          Sprite->FacingMode,
					          ENiagaraSpriteFacingMode::CustomFacingVector);
				}
			}
			TestTrue(TEXT("Goat ground effect contains at least one ground-facing sprite renderer"),
			         GroundSpriteRendererCount > 0);
		}
		if (Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash)
		{
			int32 GroundSpriteRendererCount = 0;
			for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
			{
				const FVersionedNiagaraEmitterData* EmitterData =
				    EmitterHandle.GetIsEnabled() ? EmitterHandle.GetEmitterData() : nullptr;
				if (!EmitterData)
				{
					continue;
				}
				for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
				{
					const UNiagaraSpriteRendererProperties* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer);
					if (!Sprite || !Sprite->GetIsEnabled())
					{
						continue;
					}
					++GroundSpriteRendererCount;
					TestEqual(TEXT("Scythe slash sprite uses a custom ground-facing vector"),
					          Sprite->FacingMode,
					          ENiagaraSpriteFacingMode::CustomFacingVector);
					TestEqual(TEXT("Scythe slash sprite binds the authoritative ground normal"),
					          Sprite->SpriteFacingBinding.GetParamMapBindableVariable().GetName(),
					          FName(TEXT("User.GroundNormal")));
				}
			}
			TestTrue(TEXT("Scythe slash contains at least one ground-facing sprite renderer"),
			         GroundSpriteRendererCount > 0);
		}
		if (Semantic == EReEchoCombatVfxSemantic::GoatSkill04Lighting)
		{
			int32 BeamMeshRendererCount = 0;
			for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
			{
				const FVersionedNiagaraEmitterData* EmitterData =
				    EmitterHandle.GetIsEnabled() ? EmitterHandle.GetEmitterData() : nullptr;
				if (!EmitterData)
				{
					continue;
				}
				for (const UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
				{
					const UNiagaraMeshRendererProperties* Mesh = Cast<UNiagaraMeshRendererProperties>(Renderer);
					if (!Mesh)
					{
						continue;
					}
					++BeamMeshRendererCount;
					TestEqual(TEXT("Goat beam mesh faces the camera plane"),
					          Mesh->FacingMode,
					          ENiagaraMeshFacingMode::CameraPlane);
					TestTrue(TEXT("Goat beam mesh locks its camera-facing rotation"), Mesh->bLockedAxisEnable);
					TestTrue(TEXT("Goat beam mesh remains upright around world Z"),
					         Mesh->LockedAxis.Equals(FVector::UpVector, KINDA_SMALL_NUMBER));
					TestEqual(TEXT("Goat beam mesh lock is evaluated in world space"),
					          Mesh->LockedAxisSpace,
					          ENiagaraMeshLockedAxisSpace::World);
				}
			}
			TestTrue(TEXT("Goat beam contains at least one camera-facing mesh renderer"), BeamMeshRendererCount > 0);
		}
	}
	const TCHAR* FireSystems[] = {
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.Slime")),
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.Rabbit")),
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.Fox")),
	};
	for (const TCHAR* FirePath : FireSystems)
	{
		UNiagaraSystem* FireSystem = LoadObject<UNiagaraSystem>(nullptr, FirePath);
		if (!TestNotNull(FString::Printf(TEXT("Fire Niagara loads: %s"), FirePath), FireSystem))
		{
			continue;
		}
		for (const FNiagaraEmitterHandle& EmitterHandle : FireSystem->GetEmitterHandles())
		{
			if (EmitterHandle.GetIsEnabled())
			{
				const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
				TestTrue(FString::Printf(TEXT("Attached Fire emitter '%s' uses local space"),
				                         *EmitterHandle.GetName().ToString()),
				         EmitterData && EmitterData->bLocalSpace);
			}
		}
	}
	TestNotNull(TEXT("Logic-driven rabbit projectile texture loads"),
	            LoadObject<UTexture2D>(nullptr, FReEchoCombatVfxCatalog::ResolveRabbitProjectileTexturePath()));
	TestNotNull(
	    TEXT("Logic-driven rabbit projectile keeps the authored emissive material"),
	    LoadObject<UMaterialInterface>(nullptr, FReEchoCombatVfxCatalog::ResolveRabbitProjectileMaterialPath()));
	for (int32 GlowLayerIndex = 0; GlowLayerIndex < FReEchoCombatVfxCatalog::GetRabbitProjectileGlowMaterialCount();
	     ++GlowLayerIndex)
	{
		const TCHAR* GlowPath = FReEchoCombatVfxCatalog::ResolveRabbitProjectileGlowMaterialPath(GlowLayerIndex);
		TestNotNull(FString::Printf(TEXT("Rabbit additive glow material loads: %s"), GlowPath),
		            LoadObject<UMaterialInterface>(nullptr, GlowPath));
	}

	UNiagaraSystem* RabbitProjectileSystem = LoadObject<UNiagaraSystem>(
	    nullptr, *FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::RabbitProjectile));
	if (TestNotNull(TEXT("Rabbit projectile Niagara system loads for emitter-space validation"),
	                RabbitProjectileSystem))
	{
		for (const FNiagaraEmitterHandle& EmitterHandle : RabbitProjectileSystem->GetEmitterHandles())
		{
			if (!EmitterHandle.GetIsEnabled())
			{
				continue;
			}
			const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
			TestTrue(FString::Printf(TEXT("Rabbit projectile emitter '%s' uses local space"),
			                         *EmitterHandle.GetName().ToString()),
			         EmitterData && EmitterData->bLocalSpace);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionVfxLifetimeTest,
                                 "ReEcho.Presentation.VFX.ElementReactionLifetime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionVfxLifetimeTest::RunTest(const FString& Parameters)
{
	UNiagaraSystem* GrassSystem = LoadObject<UNiagaraSystem>(
	    nullptr, FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceGrass));
	if (TestNotNull(TEXT("Grass reaction Niagara loads"), GrassSystem))
	{
		for (const FNiagaraEmitterHandle& EmitterHandle : GrassSystem->GetEmitterHandles())
		{
			if (!EmitterHandle.GetIsEnabled())
			{
				continue;
			}
			const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
			TestTrue(
			    FString::Printf(TEXT("Grass reaction emitter '%s' uses local space so reaction scale controls range"),
			                    *EmitterHandle.GetName().ToString()),
			    EmitterData && EmitterData->bLocalSpace);
		}
	}
	const TCHAR* GoatFirePath =
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TEXT("Enemy.TimeGuard"));
	const TCHAR* RabbitWaterPath = FReEchoElementReactionVfxCatalog::ResolvePath(
	    EReEchoElementReactionVfxSemantic::Vaporize, TEXT("Enemy.Rabbit"));
	const TCHAR* GoatWaterPath = FReEchoElementReactionVfxCatalog::ResolvePath(
	    EReEchoElementReactionVfxSemantic::Vaporize, TEXT("Enemy.TimeGuard"));
	TestNotNull(TEXT("Goat Fire reaction Niagara loads"), LoadObject<UNiagaraSystem>(nullptr, GoatFirePath));
	UNiagaraSystem* RabbitWaterSystem = LoadObject<UNiagaraSystem>(nullptr, RabbitWaterPath);
	TestNotNull(TEXT("Rabbit Water reaction Niagara loads"), RabbitWaterSystem);
	UNiagaraSystem* GoatFireSystem = LoadObject<UNiagaraSystem>(nullptr, GoatFirePath);
	UNiagaraSystem* GoatWaterSystem = LoadObject<UNiagaraSystem>(nullptr, GoatWaterPath);
	TestNotNull(TEXT("Goat Water reaction Niagara loads"), GoatWaterSystem);
	auto TestTargetBoundSystemUsesLocalSpace = [this](const TCHAR* Label, const UNiagaraSystem* System)
	{
		if (!System)
		{
			return;
		}
		for (const FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			if (!EmitterHandle.GetIsEnabled())
			{
				continue;
			}
			const FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData();
			TestTrue(FString::Printf(TEXT("%s emitter '%s' follows its Boss attachment in local space"),
			                         Label,
			                         *EmitterHandle.GetName().ToString()),
			         EmitterData && EmitterData->bLocalSpace);
		}
	};
	TestTargetBoundSystemUsesLocalSpace(TEXT("Goat Fire"), GoatFireSystem);
	TestTargetBoundSystemUsesLocalSpace(TEXT("Goat Water"), GoatWaterSystem);
	if (GrassSystem && RabbitWaterSystem)
	{
		constexpr float TestFlipbookDiameterCm = 240.0f;
		const FVector GrassScale = UReEchoCombatVfxComponent::ResolveTargetMatchedReactionWorldScale(
		    GrassSystem, TestFlipbookDiameterCm, 1.0f, FVector::OneVector);
		const FVector WaterScale = UReEchoCombatVfxComponent::ResolveTargetMatchedReactionWorldScale(
		    RabbitWaterSystem, TestFlipbookDiameterCm, 1.0f, FVector::OneVector);
		const float GrassWorldDiameter = GrassSystem->GetFixedBounds().GetSize().GetMax() * GrassScale.GetMax();
		const float WaterWorldDiameter = RabbitWaterSystem->GetFixedBounds().GetSize().GetMax() * WaterScale.GetMax();
		TestTrue(TEXT("Enhance Grass world range matches the target Flipbook"),
		         FMath::IsNearlyEqual(GrassWorldDiameter, TestFlipbookDiameterCm, 0.1f));
		TestTrue(TEXT("Enhance Water world range matches the target Flipbook"),
		         FMath::IsNearlyEqual(WaterWorldDiameter, TestFlipbookDiameterCm, 0.1f));
	}
	TestTrue(TEXT("Vaporize has a bounded reaction-only lifetime"),
	         UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(
	             static_cast<uint8>(EReEchoElementReactionVfxSemantic::Vaporize)));
	TestTrue(TEXT("Growth has a bounded reaction-only lifetime"),
	         UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(
	             static_cast<uint8>(EReEchoElementReactionVfxSemantic::Growth)));
	TestTrue(TEXT("Grass enhance has a bounded reaction-only lifetime"),
	         UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(
	             static_cast<uint8>(EReEchoElementReactionVfxSemantic::EnhanceGrass)));
	TestTrue(TEXT("Water enhance has a bounded reaction-only lifetime"),
	         UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(
	             static_cast<uint8>(EReEchoElementReactionVfxSemantic::EnhanceWater)));
	TestFalse(TEXT("Grass attachment remains state-owned instead of reaction-timed"),
	          UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(
	              static_cast<uint8>(EReEchoElementReactionVfxSemantic::AttachmentGrass)));
	TestFalse(TEXT("Water attachment remains state-owned instead of reaction-timed"),
	          UReEchoCombatVfxComponent::IsBoundedElementReactionSemantic(
	              static_cast<uint8>(EReEchoElementReactionVfxSemantic::AttachmentWater)));
	return true;
}

#endif
