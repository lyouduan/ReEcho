#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/VFX/ReEchoVfxPreviewActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoVfxPreviewValueTest,
                                 "ReEcho.Presentation.VFXPreview.Values",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoVfxPreviewValueTest::RunTest(const FString& Parameters)
{
	const FVector Source(20.0f, -40.0f, 0.0f);
	const FVector Target(-180.0f, 110.0f, 0.0f);
	const FVector Direction = AReEchoVfxPreviewActor::ResolveSafeDirection(Source, Target);
	TestTrue(TEXT("Preview direction points from Source to Target"),
	         Direction.Equals((Target - Source).GetSafeNormal2D(), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Coincident anchors use deterministic +X"),
	         AReEchoVfxPreviewActor::ResolveSafeDirection(Source, Source).Equals(FVector::ForwardVector));

	const FVector AuthoredAxis = FVector::RightVector;
	const FTransform PreviewTransform =
	    AReEchoVfxPreviewActor::ResolvePreviewTransform(Source, Target, AuthoredAxis, FTransform::Identity);
	TestTrue(
	    TEXT("Authored axis rotates onto arbitrary preview direction"),
	    PreviewTransform.TransformVectorNoScale(AuthoredAxis).GetSafeNormal2D().Equals(Direction, KINDA_SMALL_NUMBER));

	for (uint8 SemanticValue = 0; SemanticValue <= static_cast<uint8>(EReEchoCombatVfxSemantic::EnemyHurt);
	     ++SemanticValue)
	{
		TestEqual(TEXT("Combat Production preview shares catalog path"),
		          AReEchoVfxPreviewActor::ResolveProductionPath(EReEchoVfxPreviewMode::CombatSemantic,
		                                                        SemanticValue,
		                                                        0,
		                                                        NAME_None,
		                                                        NAME_None,
		                                                        EReEchoVfxPreviewWeaponSlot::Travel,
		                                                        nullptr),
		          FReEchoCombatVfxCatalog::ResolvePath(static_cast<EReEchoCombatVfxSemantic>(SemanticValue)));
	}
	TestEqual(TEXT("Element Production preview shares catalog path"),
	          AReEchoVfxPreviewActor::ResolveProductionPath(EReEchoVfxPreviewMode::ElementSemantic,
	                                                        0,
	                                                        static_cast<uint8>(EReEchoElementReactionVfxSemantic::Burn),
	                                                        TEXT("Enemy.Fox"),
	                                                        NAME_None,
	                                                        EReEchoVfxPreviewWeaponSlot::Travel,
	                                                        nullptr),
	          FString(FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Burn,
	                                                                TEXT("Enemy.Fox"))));
	TestTrue(TEXT("Invalid Production semantic remains Missing instead of borrowing an effect"),
	         AReEchoVfxPreviewActor::ResolveProductionPath(EReEchoVfxPreviewMode::CombatSemantic,
	                                                       255,
	                                                       0,
	                                                       NAME_None,
	                                                       NAME_None,
	                                                       EReEchoVfxPreviewWeaponSlot::Travel,
	                                                       nullptr)
	             .IsEmpty());
	const AReEchoVfxPreviewActor* Defaults = GetDefault<AReEchoVfxPreviewActor>();
	TestFalse(TEXT("PIE initialization does not auto-release by default"), Defaults->bAutoReleaseOnBeginPlay);
	TestEqual(TEXT("Camera zoom clamps to the minimum"), AReEchoVfxPreviewActor::ClampScenarioOrthoWidth(1.0f), 320.0f);
	TestEqual(
	    TEXT("Camera zoom clamps to the maximum"), AReEchoVfxPreviewActor::ClampScenarioOrthoWidth(50000.0f), 10000.0f);

	FReEchoElementReactionResolvedEvent ResolvedEvent;
	ResolvedEvent.ReactionId = TEXT("Y_ER_L_W");
	ResolvedEvent.ReactionLinks.AddDefaulted(3);
	const TArray<FReEchoElementReactionLink> Consumed =
	    AReEchoVfxPreviewActor::ConsumeResolvedReactionLinks(ResolvedEvent);
	TestEqual(TEXT("Scenario consumes every authoritative ReactionLink"), Consumed.Num(), 3);
	TestEqual(TEXT("Scenario consumption never mutates the production event"), ResolvedEvent.ReactionLinks.Num(), 3);
	TestTrue(TEXT("Scenario keeps authoritative link order"),
	         &Consumed[0] != &ResolvedEvent.ReactionLinks[0] &&
	             Consumed[0].SourceTarget == ResolvedEvent.ReactionLinks[0].SourceTarget);
	TArray<FReEchoVfxScenarioLink> AuthoredLinks;
	AuthoredLinks.AddDefaulted(4);
	const TArray<FReEchoElementReactionLink> FormalLinks =
	    AReEchoVfxPreviewActor::ConvertAuthoredReactionLinks(AuthoredLinks);
	TestEqual(TEXT("Editable links map one-to-one into formal ReactionLinks"), FormalLinks.Num(), 4);

	UWorld* PreviewWorld = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	AReEchoVfxPreviewActor* Rig = PreviewWorld ? PreviewWorld->SpawnActor<AReEchoVfxPreviewActor>() : nullptr;
	TestNotNull(TEXT("Scenario rig can exist in an isolated editor-preview world"), Rig);
	if (Rig)
	{
		Rig->AddScenarioTarget();
		Rig->AddScenarioTarget();
		TestEqual(TEXT("Multi-target instance add is deterministic"), Rig->GetCalibrationTargetCountForTests(), 2);
		Rig->RemoveScenarioTarget();
		TestEqual(TEXT("Remove clears one calibration target"), Rig->GetCalibrationTargetCountForTests(), 1);
		Rig->ScenarioPreset = EReEchoVfxScenarioPreset::ConductChain;
		Rig->AuthoredReactionLinks.AddDefaulted();
		Rig->RunScenario();
		TestTrue(TEXT("Sandbox-authored links never enter the Production resolved event"),
		         Rig->ResolvedReactionEvent.ReactionLinks.IsEmpty());
		Rig->CameraPan = FVector(50.0f, 60.0f, 70.0f);
		Rig->CameraRotation = FRotator(1.0f, 2.0f, 3.0f);
		Rig->CameraOrthoWidth = 999.0f;
		const int32 TargetCountBeforeCameraReset = Rig->GetCalibrationTargetCountForTests();
		Rig->ResetScenarioCamera();
		TestTrue(TEXT("Camera reset restores the formal tilted orthographic defaults"),
		         Rig->CameraPan.Equals(FVector(-900.0f, 0.0f, 900.0f)) &&
		             Rig->CameraRotation.Equals(FRotator(-45.0f, 0.0f, 0.0f)) &&
		             FMath::IsNearlyEqual(Rig->CameraOrthoWidth, 2560.0f));
		TestEqual(TEXT("Camera controls do not mutate scenario targets"),
		          Rig->GetCalibrationTargetCountForTests(),
		          TargetCountBeforeCameraReset);
		Rig->ReleaseProductionVfx();
		TestEqual(TEXT("Editor preview Release cannot increment Production release count"), Rig->ReleaseCount, 0);
		Rig->ResetScenario();
		TestTrue(TEXT("Reset clears calibration targets without creating Production links"),
		         Rig->GetCalibrationTargetCountForTests() == 0 && Rig->ResolvedReactionEvent.ReactionLinks.IsEmpty() &&
		             Rig->AuthoredReactionLinks.IsEmpty());
	}
	if (PreviewWorld)
	{
		PreviewWorld->DestroyWorld(false);
	}
	return true;
}

#endif
