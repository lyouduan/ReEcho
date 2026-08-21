#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Misc/AutomationTest.h"
#include "PaperFlipbook.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSpadeAppearanceTest,
                                 "ReEcho.Player.SpadeAppearance",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSpadeAppearanceTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	AReEchoPlayerPawn* Player = NewObject<AReEchoPlayerPawn>(GetTransientPackage());
	TestTrue(TEXT("Sage character configures"), Player->ConfigureCharacter(TEXT("J_SPADE")));
	TestNotNull(TEXT("Sage has a stable portrait fallback"), Player->GetPortraitTexture());
	TestNotNull(TEXT("Sage resolves an active Flipbook"), Player->SequenceAnimation->GetFlipbook());

	AReEchoEchoActor* Echo = NewObject<AReEchoEchoActor>(GetTransientPackage());
	TestTrue(TEXT("Sage Echo configures"), Echo->ConfigureEchoAppearance(TEXT("J_SPADE")));

	UReEcho2DPresentationCatalog* Catalog = LoadObject<UReEcho2DPresentationCatalog>(
	    nullptr, TEXT("/Game/ReEcho/Animation2D/DA_PresentationCatalog.DA_PresentationCatalog"));
	const UReEcho2DCharacterPresentationProfile* Profile = Catalog ? Catalog->ResolveProfile(TEXT("J_SPADE")) : nullptr;
	TestNotNull(TEXT("Catalog resolves Sage profile"), Profile);
	const FReEcho2DAnimationClip* MoonStaffAttack =
	    Profile ? Profile->ResolveClip(TEXT("MoonStaff"), ReEcho2DAnimationTags::Attack_Basic) : nullptr;
	TestTrue(TEXT("Sage MoonStaff attack has a Flipbook"),
	         MoonStaffAttack && IsValid(MoonStaffAttack->Flipbook.Get()) && !MoonStaffAttack->bLooping);
	return true;
}

#endif
