#if WITH_DEV_AUTOMATION_TESTS

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
	TestTrue(TEXT("Burn visual lifetime follows the timed Burn status"),
	         UReEchoCombatVfxComponent::IsElementReactionStateDriven(TEXT("Y_ER_F_G")));
	TestTrue(TEXT("Growth visual lifetime follows authoritative Grass attachments"),
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
	TestTrue(TEXT("World-space Conduct preserves oblique direction and distance"),
	         EndParameter.Equals(EndWorld, KINDA_SMALL_NUMBER));
	FVector BeamStart = FVector::ZeroVector;
	FVector BeamEnd = FVector::ZeroVector;
	const FVector BeamWarningCenter(300.0f, -120.0f, 40.0f);
	const FVector BeamDirection(0.6f, 0.8f, 0.0f);
	UReEchoCombatVfxComponent::ResolveBossBeamWorldEndpoints(
	    BeamWarningCenter, BeamDirection, 750.0f, BeamStart, BeamEnd);
	TestTrue(TEXT("Boss beam starts at the authoritative warning center"),
	         BeamStart.Equals(BeamWarningCenter, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Boss beam endpoint extends upward from the warning center"),
	         BeamEnd.Equals(BeamWarningCenter + FVector::ForwardVector * 750.0f, KINDA_SMALL_NUMBER));
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
	TestTrue(TEXT("Ordinary attached VFX retains its configured relative scale"),
	         UReEchoCombatVfxComponent::ResolveAttachedScale(FVector(1.2f, 0.8f, 1.0f), FVector(2.0f), false)
	             .Equals(FVector(1.2f, 0.8f, 1.0f), KINDA_SMALL_NUMBER));
	const FReEchoVfxPlacement SwordPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::PlayerMeleeSlash);
	TestTrue(TEXT("Sword slash uses the committed world attack direction"), SwordPlacement.bUseWorldDirectionRotation);
	TestEqual(TEXT("Forward longsword slash releases its VFX immediately"),
	          FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(EReEchoCombatVfxSemantic::PlayerMeleeSlash),
	          0.0f);
	TestTrue(TEXT("Scythe slash waits for its full-spin motion"),
	         FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(EReEchoCombatVfxSemantic::PlayerScytheSlash) > 0.0f);
	TestTrue(TEXT("Sword slash placement comes from its weapon profile"),
	         SwordPlacement.LocalOffset.Equals(FVector(0.0f, 0.0f, 60.0f), KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Sword slash consumes a finite artist-authored rotation"),
	          SwordPlacement.LocalRotation.ContainsNaN());
	TestTrue(TEXT("Sword slash corrects the replacement asset's reversed authored axis"),
	         FMath::IsNearlyEqual(FMath::Abs(SwordPlacement.LocalRotation.Yaw), 180.0f, KINDA_SMALL_NUMBER));
	TestTrue(
	    TEXT("Sword slash cancels different host scales"),
	    UReEchoCombatVfxComponent::ResolveAttachedScale(
	        SwordPlacement.Scale, FVector(2.0f), SwordPlacement.ScalePolicy == EReEchoVfxScalePolicy::PreserveWorldSize)
	        .Equals(SwordPlacement.Scale * 0.5f, KINDA_SMALL_NUMBER));
	const FVector SlashDirections[] = {FVector::ForwardVector, FVector::BackwardVector, FVector(0.6f, 0.8f, 0.0f)};
	const FVector CameraFacingNormal(-0.573576f, 0.0f, 0.819152f);
	TestEqual(TEXT("Left-side sword slash plays forward"),
	          UReEchoCombatVfxComponent::ResolveMeleePlayDirection(-FVector::RightVector, FVector::RightVector),
	          1.0f);
	TestEqual(TEXT("Right-side sword slash plays in reverse"),
	          UReEchoCombatVfxComponent::ResolveMeleePlayDirection(FVector::RightVector, FVector::RightVector),
	          -1.0f);
	for (const FVector& SlashDirection : SlashDirections)
	{
		const FRotator DirectionRotation =
		    UReEchoCombatVfxComponent::ResolveCameraPlaneDirectionRotation(SlashDirection, CameraFacingNormal);
		const FVector ExpectedPlaneDirection =
		    (SlashDirection - FVector::DotProduct(SlashDirection, CameraFacingNormal) * CameraFacingNormal)
		        .GetSafeNormal();
		TestTrue(
		    TEXT("Scythe-style VFX rotates in the camera-facing plane toward the committed enemy"),
		    DirectionRotation.RotateVector(FVector::ForwardVector).Equals(ExpectedPlaneDirection, KINDA_SMALL_NUMBER));
		const FRotator SwordDirectionRotation =
		    UReEchoCombatVfxComponent::ResolveSwordMeshDirectionRotation(SlashDirection, CameraFacingNormal);
		TestTrue(TEXT("Sword slash presents its authored local X surface normal to the camera"),
		         SwordDirectionRotation.RotateVector(FVector::ForwardVector)
		             .Equals(CameraFacingNormal.GetSafeNormal(), KINDA_SMALL_NUMBER));
		const FRotator ComposedSwordRotation =
		    UReEchoCombatVfxComponent::ComposeAttachedRotation(SwordDirectionRotation, SwordPlacement.LocalRotation);
		const FVector ComposedAttackAxis = ComposedSwordRotation.RotateVector(FVector::RightVector);
		const FRotator FrontFacingSwordRotation =
		    UReEchoCombatVfxComponent::EnsureSwordFrontFacesCamera(ComposedSwordRotation, CameraFacingNormal);
		TestTrue(TEXT("Sword DA correction cannot leave the rendered surface back-facing"),
		         FVector::DotProduct(FrontFacingSwordRotation.RotateVector(FVector::ForwardVector),
		                             CameraFacingNormal.GetSafeNormal()) >= 0.0f);
		TestTrue(
		    TEXT("Sword front-face correction preserves its composed attack axis"),
		    FrontFacingSwordRotation.RotateVector(FVector::RightVector).Equals(ComposedAttackAxis, KINDA_SMALL_NUMBER));
	}
	const FVector MovedEndWorld(-240.0f, 910.0f, 25.0f);
	UReEchoCombatVfxComponent::ResolveConductLinkWorldEndpoints(
	    StartWorld, MovedEndWorld, StartParameter, EndParameter);
	TestTrue(TEXT("Moved target is recomputed rather than retaining the event snapshot"),
	         EndParameter.Equals(MovedEndWorld, KINDA_SMALL_NUMBER));
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
	const FReEchoVfxPlacement FoxDirectionPlacement =
	    FReEchoCombatVfxCatalog::ResolvePlacement(EReEchoCombatVfxSemantic::FoxDirection);
	TestTrue(TEXT("Fox windup arrow keeps a visible non-degenerate component scale"),
	         FoxDirectionPlacement.Scale.GetAbsMin() > KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Fox windup arrow uses the combat foreground sort band"),
	         UReEchoCombatVfxComponent::ResolveCombatEffectSortPriority(0) >= 1000);
	TestFalse(TEXT("Gun impact production slot resolves a configured Niagara path"),
	          FReEchoCombatVfxCatalog::ResolvePath(EReEchoCombatVfxSemantic::PlayerGunImpact).IsEmpty());

	const EReEchoCombatVfxSemantic RequiredSystems[] = {
	    EReEchoCombatVfxSemantic::RabbitCharging,      EReEchoCombatVfxSemantic::RabbitProjectile,
	    EReEchoCombatVfxSemantic::PlayerHurt,          EReEchoCombatVfxSemantic::FoxCharging,
	    EReEchoCombatVfxSemantic::FoxDirection,        EReEchoCombatVfxSemantic::FoxDash,
	    EReEchoCombatVfxSemantic::FoxImpact,           EReEchoCombatVfxSemantic::PlayerMeleeSlash,
	    EReEchoCombatVfxSemantic::PlayerScytheSlash,   EReEchoCombatVfxSemantic::PlayerLongSwordImpact,
	    EReEchoCombatVfxSemantic::PlayerScytheImpact,  EReEchoCombatVfxSemantic::PlayerBowFlight,
	    EReEchoCombatVfxSemantic::PlayerBowImpact,     EReEchoCombatVfxSemantic::PlayerGunFlight,
	    EReEchoCombatVfxSemantic::PlayerGunImpact,     EReEchoCombatVfxSemantic::EnemyHurt,
	    EReEchoCombatVfxSemantic::EchoWaterAura,       EReEchoCombatVfxSemantic::EchoGrassAura,
	    EReEchoCombatVfxSemantic::GoatSkill02Charging, EReEchoCombatVfxSemantic::GoatSkill02Bullet,
	    EReEchoCombatVfxSemantic::GoatSkill02Impact,   EReEchoCombatVfxSemantic::GoatSkill03Charging,
	    EReEchoCombatVfxSemantic::GoatSkill03Alarming, EReEchoCombatVfxSemantic::GoatSkill03Impact,
	    EReEchoCombatVfxSemantic::GoatSkill04Charging, EReEchoCombatVfxSemantic::GoatSkill04Lighting,
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
		const bool bRequiresComponentSpace =
		    Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight ||
		    Semantic == EReEchoCombatVfxSemantic::PlayerGunFlight ||
		    Semantic == EReEchoCombatVfxSemantic::FoxDirection || Semantic == EReEchoCombatVfxSemantic::FoxDash ||
		    Semantic == EReEchoCombatVfxSemantic::EchoWaterAura || Semantic == EReEchoCombatVfxSemantic::EchoGrassAura;
		const bool bRequiresWeaponLocalSpace = Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight ||
		                                       Semantic == EReEchoCombatVfxSemantic::PlayerGunFlight;
		if (bRequiresComponentSpace)
		{
			int32 BowSpriteRendererCount = 0;
			int32 SwordMeshRendererCount = 0;
			int32 FoxDirectionEnabledEmitterCount = 0;
			int32 FoxDirectionEnabledRendererCount = 0;
			int32 FoxDirectionEnabledSpriteRendererCount = 0;
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
				if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash && EmitterData)
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
			if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash)
			{
				TestTrue(TEXT("Sword slash retains at least one authored mesh renderer"), SwordMeshRendererCount > 0);
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

#endif
