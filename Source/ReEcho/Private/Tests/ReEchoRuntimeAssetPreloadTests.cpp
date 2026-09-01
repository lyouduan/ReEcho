#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Presentation/Loading/ReEchoRuntimeAssetPreloader.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuntimeAssetPreloadCatalogTest,
                                 "ReEcho.Presentation.RuntimeAssetPreload.Catalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuntimeAssetPreloadCatalogTest::RunTest(const FString& Parameters)
{
	const TArray<FSoftObjectPath> AssetPaths = UReEchoRuntimeAssetPreloader::BuildDefaultAssetList();
	TArray<FString> GatheredPaths;
	FReEchoCombatVfxCatalog::GatherPreloadAssetPaths(GatheredPaths);
	FReEchoElementReactionVfxCatalog::GatherPreloadAssetPaths(GatheredPaths);
	FReEchoWeaponVisualCatalog::GatherPreloadAssetPaths(GatheredPaths);
	AReEchoDamageNumberActor::GatherPreloadAssetPaths(GatheredPaths);
	const TArray<FSoftObjectPath> ExpectedPaths = UReEchoRuntimeAssetPreloader::NormalizeAssetPaths(GatheredPaths);
	TestEqual(TEXT("Default preload count matches every authoritative presentation provider"),
	          AssetPaths.Num(),
	          ExpectedPaths.Num());
	for (int32 PathIndex = 0; PathIndex < FMath::Min(AssetPaths.Num(), ExpectedPaths.Num()); ++PathIndex)
	{
		TestTrue(FString::Printf(TEXT("Default preload path %d preserves authoritative catalog order"), PathIndex),
		         AssetPaths[PathIndex] == ExpectedPaths[PathIndex]);
	}
	TestFalse(TEXT("The authoritative preload list is not empty"), AssetPaths.IsEmpty());
	TSet<FSoftObjectPath> UniquePaths;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		TestTrue(TEXT("Every preload path is valid"), AssetPath.IsValid());
		TestFalse(TEXT("Every preload path is unique"), UniquePaths.Contains(AssetPath));
		UniquePaths.Add(AssetPath);
	}
	TestTrue(TEXT("Damage-number Blueprint class is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(AReEchoDamageNumberActor::GetDamageNumberBlueprintClassPath())));
	TestTrue(TEXT("Damage-number font is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(AReEchoDamageNumberActor::GetDamageNumberFontPath())));
	TestTrue(TEXT("Damage-number material is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(AReEchoDamageNumberActor::GetDamageNumberMaterialPath())));
	TestTrue(TEXT("Longsword Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01.NS_People_Sword_Attack_01"))));
	const TCHAR* LongSwordElementSlashPaths[] = {
	    TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Fire.NS_People_Sword_Attack_Fire"),
	    TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Thunder.NS_People_Sword_Attack_Thunder"),
	    TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Grass.NS_People_Sword_Attack_Grass"),
	    TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_Water.NS_People_Sword_Attack_Water"),
	};
	for (const TCHAR* LongSwordElementSlashPath : LongSwordElementSlashPaths)
	{
		TestTrue(TEXT("Element-specific Longsword slash Niagara is preloaded"),
		         UniquePaths.Contains(FSoftObjectPath(LongSwordElementSlashPath)));
	}
	TestTrue(TEXT("Scythe Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_01.NS_People_Sickle_Attack_01"))));
	const TCHAR* ScytheElementSlashPaths[] = {
	    TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Fire.NS_People_Sickle_Attack_Fire"),
	    TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Thunder.NS_People_Sickle_Attack_Thunder"),
	    TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Grass.NS_People_Sickle_Attack_Grass"),
	    TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_Water.NS_People_Sickle_Attack_Water"),
	};
	for (const TCHAR* ScytheElementSlashPath : ScytheElementSlashPaths)
	{
		TestTrue(TEXT("Element-specific Scythe slash Niagara is preloaded"),
		         UniquePaths.Contains(FSoftObjectPath(ScytheElementSlashPath)));
	}
	TestTrue(TEXT("Bow flight Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_01.NS_People_Bow_Attack_01"))));
	const TCHAR* BowElementFlightPaths[] = {
	    TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Fire.NS_People_Bow_Attack_Fire"),
	    TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Thunder.NS_People_Bow_Attack_Thunder"),
	    TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Grass.NS_People_Bow_Attack_Grass"),
	    TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_Water.NS_People_Bow_Attack_Water"),
	};
	for (const TCHAR* BowElementFlightPath : BowElementFlightPaths)
	{
		TestTrue(TEXT("Element-specific Bow flight Niagara is preloaded"),
		         UniquePaths.Contains(FSoftObjectPath(BowElementFlightPath)));
	}
	TestTrue(TEXT("Gun muzzle Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark.NS_People_Bullet_spark"))));
	const TCHAR* GunElementFlightPaths[] = {
	    TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Fire_Fly.NS_People_Bullet_Fire_Fly"),
	    TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Thunder_Fly.NS_People_Bullet_Thunder_Fly"),
	    TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Grass_Fly.NS_People_Bullet_Grass_Fly"),
	    TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_Water_Fly.NS_People_Bullet_Water_Fly"),
	};
	for (const TCHAR* GunElementFlightPath : GunElementFlightPaths)
	{
		TestTrue(TEXT("Element-specific Gun flight Niagara is preloaded"),
		         UniquePaths.Contains(FSoftObjectPath(GunElementFlightPath)));
	}
	TestFalse(TEXT("Retired Whip presentation profile is not preloaded"),
	          UniquePaths.Contains(FSoftObjectPath(TEXT(
	              "/Game/ReEcho/DataAsset/Weapon/Profiles/DA_WeaponPresentation_Whip.DA_WeaponPresentation_Whip"))));
	TestTrue(TEXT("Grass attachment Niagara is preloaded"),
	         UniquePaths.Contains(
	             FSoftObjectPath(TEXT("/Game/VFX/Element/Grass/Particle/NS_Element_Grass.NS_Element_Grass"))));
	TestTrue(TEXT("Conduct Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/Element/Elctricity/Particle/NS_Element_Electricity.NS_Element_Electricity"))));
	TestTrue(TEXT("Echo Water aura Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(TEXT("/Game/VFX/Echo/Particle/NS_Echo_Water.NS_Echo_Water"))));
	TestTrue(TEXT("Echo Grass aura Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(TEXT("/Game/VFX/Echo/Particle/NS_Echo_Grass.NS_Echo_Grass"))));
	TestTrue(TEXT("Sheep Skill02 bullet Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet.NS_Goat_Skill02_Bullet"))));
	TestTrue(TEXT("Sheep Skill03 alarming Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming.NS_Goat_Skill03_Alarming"))));
	TestTrue(TEXT("Sheep Skill04 lighting Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting.NS_Goat_Skill04_Lighting"))));

	const TArray<FString> DirtyPaths = {TEXT("  /Game/ReEcho/Textures/Effects/Bow.Bow  "),
	                                    TEXT("/Game/ReEcho/Textures/Effects/Bow.Bow"),
	                                    TEXT(""),
	                                    TEXT("   ")};
	const TArray<FSoftObjectPath> Normalized = UReEchoRuntimeAssetPreloader::NormalizeAssetPaths(DirtyPaths);
	TestEqual(TEXT("Normalization trims, removes empty values and deduplicates"), Normalized.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuntimeAssetPreloadCompletionTest,
                                 "ReEcho.Presentation.RuntimeAssetPreload.Completion",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuntimeAssetPreloadCompletionTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRuntimeAssetPreloader* EmptyPreloader = NewObject<UReEchoRuntimeAssetPreloader>(GameInstance);
	EmptyPreloader->BeginPreloadForTests({});
	int32 EmptyCallbackCount = 0;
	EmptyPreloader->RequestPreload(FSimpleDelegate::CreateLambda(
	    [&EmptyCallbackCount]()
	    {
		    ++EmptyCallbackCount;
	    }));
	EmptyPreloader->CompletePreloadForTests(true);
	TestTrue(TEXT("An empty list completes successfully"), EmptyPreloader->DidPreloadSucceed());
	TestEqual(TEXT("Completion is idempotent and invokes the callback once"), EmptyCallbackCount, 1);

	UReEchoRuntimeAssetPreloader* FailedPreloader = NewObject<UReEchoRuntimeAssetPreloader>(GameInstance);
	FailedPreloader->CompletePreloadForTests(false);
	int32 FailureCallbackCount = 0;
	FailedPreloader->RequestPreload(FSimpleDelegate::CreateLambda(
	    [&FailureCallbackCount]()
	    {
		    ++FailureCallbackCount;
	    }));
	FailedPreloader->CompletePreloadForTests(false);
	TestTrue(TEXT("A failed request still reaches a terminal state"), FailedPreloader->IsPreloadComplete());
	TestFalse(TEXT("Failure remains observable"), FailedPreloader->DidPreloadSucceed());
	TestEqual(TEXT("Failure releases waiting callers exactly once"), FailureCallbackCount, 1);

	GameInstance->MarkAsGarbage();
	return true;
}

#endif
