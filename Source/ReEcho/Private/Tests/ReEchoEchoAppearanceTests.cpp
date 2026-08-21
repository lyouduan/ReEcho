#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Misc/AutomationTest.h"
#include "PaperFlipbook.h"
#include "Presentation/Animation2D/ReEcho2DAnimationComponent.h"
#include "Presentation/Animation2D/ReEcho2DAnimationTags.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "Presentation/Animation2D/ReEcho2DPresentationCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEchoAppearanceMappingTest,
                                 "ReEcho.Presentation.EchoAppearance.CharacterMappings",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEchoAppearanceMappingTest::RunTest(const FString& Parameters)
{
	struct FExpectedMapping
	{
		FName CharacterId;
		const TCHAR* WalkPath;
		const TCHAR* AttackPath;
		const TCHAR* RangedAttackPath;
	};
	const FExpectedMapping Mappings[] = {
	    {TEXT("J_HEART"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Heart/Flipbooks/walk.walk"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Heart/Flipbooks/Attack.Attack"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Heart/Flipbooks/Attack_Arrow.Attack_Arrow")},
	    {TEXT("J_SPADE"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Spade/Flipbooks/walk.walk"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Spade/Flipbooks/Attack.Attack"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Spade/Flipbooks/Attack_Arrow.Attack_Arrow")},
	    {TEXT("J_CLOVER"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Clover/Flipbooks/walk.walk"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Clover/Flipbooks/Attack.Attack"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Clover/Flipbooks/Attack_Arrow.Attack_Arrow")},
	    {TEXT("J_DIAMOND"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Diamond/Flipbooks/Walk.Walk"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Diamond/Flipbooks/Attack.Attack"), TEXT("/Game/ReEcho/Art/Animation2D/Echos/Diamond/Flipbooks/Attack_Arrow.Attack_Arrow")},
	};
	UReEcho2DPresentationCatalog* Catalog = LoadObject<UReEcho2DPresentationCatalog>(nullptr, TEXT("/Game/ReEcho/Animation2D/DA_EchoPresentationCatalog.DA_EchoPresentationCatalog"));
	if (!TestNotNull(TEXT("Echo presentation catalog loads"), Catalog))
	{
		return false;
	}
	AReEchoEchoActor* Echo = NewObject<AReEchoEchoActor>(GetTransientPackage());
	UReEcho2DAnimationComponent* Animation = Echo->FindComponentByClass<UReEcho2DAnimationComponent>();
	if (!TestNotNull(TEXT("Echo has a Paper2D animation renderer"), Animation))
	{
		return false;
	}
	for (const FExpectedMapping& Mapping : Mappings)
	{
		UPaperFlipbook* Walk = LoadObject<UPaperFlipbook>(nullptr, Mapping.WalkPath);
		UPaperFlipbook* Attack = LoadObject<UPaperFlipbook>(nullptr, Mapping.AttackPath);
		UPaperFlipbook* RangedAttack = LoadObject<UPaperFlipbook>(nullptr, Mapping.RangedAttackPath);
		const UReEcho2DCharacterPresentationProfile* Profile = Catalog->ResolveProfile(Mapping.CharacterId);
		TestNotNull(*FString::Printf(TEXT("%s profile resolves"), *Mapping.CharacterId.ToString()), Profile);
		TestNotNull(*FString::Printf(TEXT("%s walk loads"), *Mapping.CharacterId.ToString()), Walk);
		TestNotNull(*FString::Printf(TEXT("%s attack loads"), *Mapping.CharacterId.ToString()), Attack);
		TestNotNull(*FString::Printf(TEXT("%s ranged attack loads"), *Mapping.CharacterId.ToString()), RangedAttack);
		const FReEcho2DAnimationClip* MeleeClip = Profile ? Profile->ResolveClip(TEXT("CrescentBlade"), ReEcho2DAnimationTags::Attack_Basic) : nullptr;
		const FReEcho2DAnimationClip* RangedClip = Profile ? Profile->ResolveClip(TEXT("Bow"), ReEcho2DAnimationTags::Attack_Basic) : nullptr;
		TestEqual(TEXT("Melee maps to Attack"), MeleeClip ? MeleeClip->Flipbook.Get() : nullptr, Attack);
		TestEqual(TEXT("Bow maps to Attack_Arrow"), RangedClip ? RangedClip->Flipbook.Get() : nullptr, RangedAttack);
		TestTrue(TEXT("Echo configures"), Echo->ConfigureEchoAppearance(Mapping.CharacterId));
		TestEqual(TEXT("Echo uses its own walk"), Animation->GetFlipbook(), Walk);
	}
	TestFalse(TEXT("Unknown character id is rejected"), Echo->ConfigureEchoAppearance(TEXT("J_UNKNOWN")));
	return true;
}

#endif
