#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoElementReaction.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "UI/ReEchoElementReactionPopupActor.h"
#include "UI/ReEchoEncounterHudWidget.h"
#include "UI/ReEchoMinimapCanvasWidget.h"
#include "UI/ReEchoPlayerHudWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatHudFormattingTest,
                                 "ReEcho.UI.CombatHud.Formatting",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatHudFormattingTest::RunTest(const FString& Parameters)
{
	UTexture2D* MinimapInkTip =
	    LoadObject<UTexture2D>(nullptr, UReEchoMinimapCanvasWidget::GetInkBrushTipTexturePath());
	UTexture2D* MinimapInkGrain = LoadObject<UTexture2D>(nullptr, UReEchoMinimapCanvasWidget::GetInkGrainTexturePath());
	UMaterialInterface* MinimapInkMaterial =
	    LoadObject<UMaterialInterface>(nullptr, UReEchoMinimapCanvasWidget::GetInkTrailMaterialPath());
	TestNotNull(TEXT("Minimap ink brush-tip texture loads"), MinimapInkTip);
	TestNotNull(TEXT("Minimap ink grain texture loads"), MinimapInkGrain);
	TestNotNull(TEXT("Minimap ink UI material loads"), MinimapInkMaterial);
	if (MinimapInkMaterial)
	{
		TestEqual(TEXT("Minimap ink material is translucent"),
		          MinimapInkMaterial->GetBlendMode(),
		          EBlendMode::BLEND_Translucent);
		TestEqual(TEXT("Minimap ink material uses the UI domain"),
		          static_cast<EMaterialDomain>(MinimapInkMaterial->GetMaterial()->MaterialDomain),
		          EMaterialDomain::MD_UI);
	}

	const TPair<FName, FString> ReactionPopupTextures[] = {
	    {TEXT("Reaction.Burn"), TEXT("T_UI_Reaction_Burn")},
	    {TEXT("Reaction.Vaporize"), TEXT("T_UI_Reaction_Vaporize")},
	    {TEXT("Reaction.Growth"), TEXT("T_UI_Reaction_Growth")},
	    {TEXT("Reaction.Conduct"), TEXT("T_UI_Reaction_Conduct")},
	    {TEXT("Reaction.Enhance"), TEXT("T_UI_Reaction_Enhance")},
	};
	for (const TPair<FName, FString>& ReactionPopup : ReactionPopupTextures)
	{
		const TCHAR* TexturePath = AReEchoElementReactionPopupActor::GetReactionTexturePath(ReactionPopup.Key);
		const FString MappingWhat = FString::Printf(TEXT("Reaction popup maps %s"), *ReactionPopup.Key.ToString());
		TestNotNull(*MappingWhat, TexturePath);
		if (TexturePath)
		{
			const FString PathWhat = FString::Printf(TEXT("Reaction popup path names %s"), *ReactionPopup.Value);
			TestTrue(*PathWhat, FString(TexturePath).Contains(ReactionPopup.Value));
			const FString LoadWhat = FString::Printf(TEXT("Reaction popup texture loads: %s"), TexturePath);
			TestNotNull(*LoadWhat, LoadObject<UTexture2D>(nullptr, TexturePath));
		}
	}
	TestNull(TEXT("Unknown reaction does not invent a popup asset"),
	         AReEchoElementReactionPopupActor::GetReactionTexturePath(TEXT("Reaction.Unknown")));
	TestEqual(TEXT("Reaction popup starts opaque"),
	          AReEchoElementReactionPopupActor::CalculateOpacity(0.0f, 0.15f, 1.3f),
	          1.0f);
	TestTrue(TEXT("Reaction popup fades during its lifetime"),
	         AReEchoElementReactionPopupActor::CalculateOpacity(0.6f, 0.15f, 1.3f) > 0.0f &&
	             AReEchoElementReactionPopupActor::CalculateOpacity(0.6f, 0.15f, 1.3f) < 1.0f);
	TestEqual(TEXT("Reaction popup ends transparent"),
	          AReEchoElementReactionPopupActor::CalculateOpacity(1.0f, 0.15f, 1.3f),
	          0.0f);

	UMaterialInterface* ReactionPopupMaterial =
	    LoadObject<UMaterialInterface>(nullptr, AReEchoElementReactionPopupActor::GetPopupMaterialPath());
	TestNotNull(TEXT("Reaction-popup translucent material loads"), ReactionPopupMaterial);
	if (ReactionPopupMaterial)
	{
		TestEqual(TEXT("Reaction-popup material is translucent"),
		          ReactionPopupMaterial->GetBlendMode(),
		          EBlendMode::BLEND_Translucent);
	}
	UClass* ReactionPopupBlueprintClass = LoadClass<AReEchoElementReactionPopupActor>(
	    nullptr, AReEchoElementReactionPopupActor::GetPopupBlueprintClassPath());
	TestNotNull(TEXT("Reaction-popup art settings Blueprint loads"), ReactionPopupBlueprintClass);
	if (ReactionPopupBlueprintClass)
	{
		TestTrue(TEXT("Reaction-popup Blueprint derives from the native actor"),
		         ReactionPopupBlueprintClass->IsChildOf(AReEchoElementReactionPopupActor::StaticClass()));
	}

	UMaterialInterface* DamageNumberMaterial =
	    LoadObject<UMaterialInterface>(nullptr, AReEchoDamageNumberActor::GetDamageNumberMaterialPath());
	TestNotNull(TEXT("Damage-number translucent material loads"), DamageNumberMaterial);
	if (DamageNumberMaterial)
	{
		TestEqual(TEXT("Damage-number material supports translucent distance-field glyphs"),
		          DamageNumberMaterial->GetBlendMode(),
		          EBlendMode::BLEND_Translucent);
	}

	auto TestReactionDamageColor = [this](const TCHAR* What, const FName ReactionBehaviorId, const FColor Expected)
	{
		FReEchoDamageEvent Event;
		Event.ReactionBehaviorId = ReactionBehaviorId;
		TestEqual(What, ReEchoElementReaction::GetDamageNumberColor(Event).ToFColor(true), Expected);
	};
	TestReactionDamageColor(
	    TEXT("Vaporize damage numbers use reference light blue"), TEXT("Reaction.Vaporize"), FColor(165, 203, 243));
	TestReactionDamageColor(
	    TEXT("Conduct damage numbers use reference yellow"), TEXT("Reaction.Conduct"), FColor(235, 192, 44));
	TestReactionDamageColor(
	    TEXT("Burn damage numbers use reference orange"), TEXT("Reaction.Burn"), FColor(232, 106, 18));
	TestReactionDamageColor(
	    TEXT("Growth damage numbers use reference green"), TEXT("Reaction.Growth"), FColor(146, 192, 57));
	TestReactionDamageColor(
	    TEXT("Enhanced damage numbers use reference gold"), TEXT("Reaction.Enhance"), FColor(241, 184, 76));

	FReEchoDamageEvent OrdinaryDamageEvent;
	OrdinaryDamageEvent.RawDamage = 7.0f;
	OrdinaryDamageEvent.AppliedDamage = 7.0f;
	TestEqual(TEXT("Ordinary damage numbers preserve the resolved damage"),
	          ReEchoElementReaction::GetDamageNumberValue(OrdinaryDamageEvent),
	          7.0f);
	FReEchoDamageEvent OverkillDamageEvent;
	OverkillDamageEvent.RawDamage = 20.0f;
	OverkillDamageEvent.AppliedDamage = 7.0f;
	TestEqual(TEXT("Overkill damage numbers show pre-health-clamp damage"),
	          ReEchoElementReaction::GetDamageNumberValue(OverkillDamageEvent),
	          20.0f);

	UFont* DamageNumberFont = LoadObject<UFont>(nullptr, AReEchoDamageNumberActor::GetDamageNumberFontPath());
	TestNotNull(TEXT("Damage-number actor owns the approved non-commercial runtime font"), DamageNumberFont);
	if (DamageNumberFont)
	{
		TestEqual(TEXT("Damage-number font uses TextRender-compatible offline caching"),
		          DamageNumberFont->FontCacheType,
		          EFontCacheType::Offline);
		TestTrue(TEXT("Damage-number font contains a baked glyph texture"), !DamageNumberFont->Textures.IsEmpty());
	}
	UClass* DamageNumberBlueprintClass =
	    LoadClass<AReEchoDamageNumberActor>(nullptr, AReEchoDamageNumberActor::GetDamageNumberBlueprintClassPath());
	TestNotNull(TEXT("Damage-number animation settings Blueprint loads"), DamageNumberBlueprintClass);
	if (DamageNumberBlueprintClass)
	{
		TestTrue(TEXT("Damage-number Blueprint derives from the native actor"),
		         DamageNumberBlueprintClass->IsChildOf(AReEchoDamageNumberActor::StaticClass()));
	}

	TestEqual(TEXT("Encounter label follows the visual spec"),
	          UReEchoEncounterHudWidget::FormatEncounterLabel(3).ToString(),
	          FString(TEXT("第 3 关")));
	TestEqual(TEXT("Countdown rounds up partial seconds"),
	          UReEchoEncounterHudWidget::FormatCountdown(59.1f).ToString(),
	          FString(TEXT("01:00")));
	TestEqual(TEXT("Countdown clamps negative values"),
	          UReEchoEncounterHudWidget::FormatCountdown(-1.0f).ToString(),
	          FString(TEXT("00:00")));
	TestEqual(TEXT("Countdown needle starts on the left"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(60.0f, 60.0f),
	          90.0f);
	TestEqual(TEXT("Countdown needle points down at half time"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(30.0f, 60.0f),
	          0.0f);
	TestEqual(TEXT("Countdown needle finishes on the right"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(0.0f, 60.0f),
	          -90.0f);
	TestEqual(TEXT("Countdown needle clamps remaining time above duration"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(90.0f, 60.0f),
	          90.0f);
	TestEqual(TEXT("Countdown needle safely handles an invalid duration before completion"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(10.0f, 0.0f),
	          90.0f);
	TestEqual(TEXT("Countdown needle safely handles an invalid duration at completion"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(0.0f, 0.0f),
	          -90.0f);
	TestEqual(TEXT("Boss health ratio uses authoritative current and maximum health"),
	          UReEchoEncounterHudWidget::CalculateBossHealthRatio(650.0f, 1300.0f),
	          0.5f);
	TestEqual(TEXT("Boss health ratio clamps over-heal"),
	          UReEchoEncounterHudWidget::CalculateBossHealthRatio(1400.0f, 1300.0f),
	          1.0f);
	TestEqual(TEXT("Boss health ratio safely handles an invalid maximum"),
	          UReEchoEncounterHudWidget::CalculateBossHealthRatio(650.0f, 0.0f),
	          0.0f);

	UReEchoPlayerHudWidget* PlayerHud = NewObject<UReEchoPlayerHudWidget>();
	PlayerHud->SetTimeShards(12);
	TestEqual(TEXT("Time shards retain the live run value"), PlayerHud->GetTimeShardsForTests(), 12);
	PlayerHud->SetTimeShards(-5);
	TestEqual(TEXT("Time shards cannot display a negative value"), PlayerHud->GetTimeShardsForTests(), 0);

	UClass* PlayerHudClass =
	    LoadClass<UReEchoPlayerHudWidget>(nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoPlayerHud.WBP_ReEchoPlayerHud_C"));
	TestNotNull(TEXT("Authored Player HUD class loads"), PlayerHudClass);
	if (!PlayerHudClass)
	{
		return false;
	}
	UReEchoPlayerHudWidget* AuthoredPlayerHud =
	    NewObject<UReEchoPlayerHudWidget>(GetTransientPackage(), PlayerHudClass);
	TestTrue(TEXT("Authored Player HUD initializes"), AuthoredPlayerHud->Initialize());
	AuthoredPlayerHud->TakeWidget();
	AuthoredPlayerHud->SetTimeShards(27);
	UTextBlock* TimeShardText = Cast<UTextBlock>(AuthoredPlayerHud->GetWidgetFromName(TEXT("TimeShardText")));
	TestNotNull(TEXT("Player HUD exposes the time-shard binding"), TimeShardText);
	if (TimeShardText)
	{
		TestEqual(TEXT("Time-shard binding renders the live balance"),
		          TimeShardText->GetText().ToString(),
		          FString(TEXT("27")));
		AuthoredPlayerHud->SetTimeShards(22);
		TestEqual(TEXT("Time-shard binding refreshes while an overlay pauses normal gameplay ticks"),
		          TimeShardText->GetText().ToString(),
		          FString(TEXT("22")));
	}
	TestNotNull(TEXT("Player HUD exposes the image health fill"),
	            Cast<UImage>(AuthoredPlayerHud->GetWidgetFromName(TEXT("PlayerHealthFill"))));
	UImage* PlayerHealthFrame = Cast<UImage>(AuthoredPlayerHud->GetWidgetFromName(TEXT("ArtHealthFrame")));
	TestNotNull(TEXT("Player HUD exposes the authored health frame"), PlayerHealthFrame);
	USizeBox* PortraitSize = Cast<USizeBox>(AuthoredPlayerHud->GetWidgetFromName(TEXT("PlayerPortraitSize")));
	TestNotNull(TEXT("Legacy portrait host is retained for compatibility"), PortraitSize);
	if (PortraitSize)
	{
		TestEqual(TEXT("Legacy portrait is absent from the authored composition"),
		          PortraitSize->GetVisibility(),
		          ESlateVisibility::Collapsed);
	}

	UClass* EncounterHudClass = LoadClass<UReEchoEncounterHudWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoEncounterHud.WBP_ReEchoEncounterHud_C"));
	TestNotNull(TEXT("Authored Encounter HUD class loads"), EncounterHudClass);
	if (!EncounterHudClass)
	{
		return false;
	}
	UReEchoEncounterHudWidget* AuthoredEncounterHud =
	    NewObject<UReEchoEncounterHudWidget>(GetTransientPackage(), EncounterHudClass);
	TestTrue(TEXT("Authored Encounter HUD initializes"), AuthoredEncounterHud->Initialize());
	AuthoredEncounterHud->TakeWidget();
	AuthoredEncounterHud->SetEncounterStatus(3, 6, 59.1f, 60.0f);
	UTextBlock* EncounterText = Cast<UTextBlock>(AuthoredEncounterHud->GetWidgetFromName(TEXT("EncounterText")));
	UTextBlock* CountdownText = Cast<UTextBlock>(AuthoredEncounterHud->GetWidgetFromName(TEXT("CountdownText")));
	UImage* ClockNeedle = Cast<UImage>(AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtClockNeedle")));
	UImage* ClockFrame = Cast<UImage>(AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtClockFrame")));
	UWidget* BossHealthPanel = AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthPanel"));
	UWidget* BossHealthFill = AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthFill"));
	UImage* BossHealthFrame = Cast<UImage>(AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthFrame")));
	TestNotNull(TEXT("Encounter HUD keeps the encounter label binding"), EncounterText);
	TestNotNull(TEXT("Encounter HUD keeps the countdown binding"), CountdownText);
	TestNotNull(TEXT("Encounter HUD keeps the animated clock needle binding"), ClockNeedle);
	TestNotNull(TEXT("Encounter HUD keeps the time frame binding"), ClockFrame);
	TestNotNull(TEXT("Encounter HUD exposes the Boss health panel"), BossHealthPanel);
	TestNotNull(TEXT("Encounter HUD exposes the Boss health fill"), BossHealthFill);
	TestNotNull(TEXT("Encounter HUD exposes the Boss health frame"), BossHealthFrame);
	if (PlayerHealthFrame && BossHealthFrame)
	{
		TestTrue(TEXT("Boss health frame reuses the Player health-frame texture"),
		         BossHealthFrame->GetBrush().GetResourceObject() == PlayerHealthFrame->GetBrush().GetResourceObject());
		const FLinearColor BossFrameTint = BossHealthFrame->GetColorAndOpacity();
		TestTrue(TEXT("Boss health frame uses a purple tint"),
		         BossFrameTint.B > BossFrameTint.R && BossFrameTint.R > BossFrameTint.G);
	}
	if (const UBorder* BossFillBorder = Cast<UBorder>(BossHealthFill))
	{
		const FLinearColor BossFillColor = BossFillBorder->GetBrushColor();
		TestTrue(TEXT("Boss health fill uses a dark purple color"),
		         BossFillColor.B > BossFillColor.R && BossFillColor.R > BossFillColor.G);
	}
	const UCanvasPanelSlot* BossFillSlot = BossHealthFill ? Cast<UCanvasPanelSlot>(BossHealthFill->Slot) : nullptr;
	const UCanvasPanelSlot* BossFrameSlot = BossHealthFrame ? Cast<UCanvasPanelSlot>(BossHealthFrame->Slot) : nullptr;
	TestNotNull(TEXT("Boss health fill has an authored Canvas slot"), BossFillSlot);
	TestNotNull(TEXT("Boss health frame has an authored Canvas slot"), BossFrameSlot);
	if (BossFillSlot && BossFrameSlot)
	{
		TestTrue(TEXT("Boss health fill draws above the opaque frame interior"),
		         BossFillSlot->GetZOrder() > BossFrameSlot->GetZOrder());
	}
	TestNull(TEXT("Rejected countdown background is absent"),
	         AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtTimeReadout")));
	if (EncounterText && CountdownText)
	{
		TestEqual(
		    TEXT("Authored encounter label refreshes"), EncounterText->GetText().ToString(), FString(TEXT("第 3 关")));
		TestEqual(TEXT("Authored countdown refreshes"), CountdownText->GetText().ToString(), FString(TEXT("01:00")));
	}
	if (ClockFrame && ClockNeedle && CountdownText && BossHealthPanel && BossHealthFill)
	{
		TestEqual(TEXT("Normal encounter shows the time frame"),
		          ClockFrame->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal encounter shows the countdown"),
		          CountdownText->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal encounter shows the clock needle"),
		          ClockNeedle->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal encounter hides the Boss health panel"),
		          BossHealthPanel->GetVisibility(),
		          ESlateVisibility::Collapsed);

		AuthoredEncounterHud->SetEncounterStatus(8, 8, 40.0f, 60.0f, true, 650.0f, 1300.0f);
		TestEqual(TEXT("Boss encounter keeps the decorative clock frame"),
		          ClockFrame->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(
		    TEXT("Boss encounter hides the countdown"), CountdownText->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(
		    TEXT("Boss encounter hides the clock needle"), ClockNeedle->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("Boss encounter shows the Boss health panel"),
		          BossHealthPanel->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Boss health fill follows the authoritative ratio"),
		          BossHealthFill->GetRenderTransform().Scale.X,
		          0.5);
	}
	TArray<UWidget*> EncounterWidgets;
	AuthoredEncounterHud->WidgetTree->GetAllWidgets(EncounterWidgets);
	const bool bHasRealMinimap = EncounterWidgets.ContainsByPredicate(
	    [](const UWidget* Widget)
	    {
		    return Cast<UReEchoMinimapCanvasWidget>(Widget) != nullptr;
	    });
	TestTrue(TEXT("Encounter HUD retains the real minimap widget"), bHasRealMinimap);
	TestNull(TEXT("Rejected bottom panel is absent from the Encounter HUD"),
	         AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtSkillBar")));

	const TCHAR* CharacterProfiles[] = {
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_HEART.DA_Character_J_HEART"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_SPADE.DA_Character_J_SPADE"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_CLOVER.DA_Character_J_CLOVER"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_DIAMOND.DA_Character_J_DIAMOND"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_HEART.DA_Echo_J_HEART"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_SPADE.DA_Echo_J_SPADE"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_CLOVER.DA_Echo_J_CLOVER"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_DIAMOND.DA_Echo_J_DIAMOND"),
	};
	for (const TCHAR* ProfilePath : CharacterProfiles)
	{
		const UReEcho2DCharacterPresentationProfile* Profile =
		    LoadObject<UReEcho2DCharacterPresentationProfile>(nullptr, ProfilePath);
		const FString ProfileLoadsWhat = FString::Printf(TEXT("Minimap profile loads: %s"), ProfilePath);
		TestNotNull(*ProfileLoadsWhat, Profile);
		if (Profile)
		{
			const FString ProfileOwnsIconWhat = FString::Printf(TEXT("Minimap profile owns an icon: %s"), ProfilePath);
			TestNotNull(*ProfileOwnsIconWhat, Profile->MinimapIcon.Get());
		}
	}

	// 暴击跳字：CriticalMultiplier 字段契约（透传链 Intent -> Resolved -> Event）。
	FReEchoHitIntent CriticalIntent;
	CriticalIntent.bCritical = true;
	CriticalIntent.CriticalMultiplier = 1.5f;
	FReEchoHitResolved CriticalResolved;
	CriticalResolved.bCritical = CriticalIntent.bCritical;
	CriticalResolved.CriticalMultiplier = CriticalIntent.CriticalMultiplier;
	FReEchoDamageEvent CriticalEvent;
	CriticalEvent.bCritical = CriticalResolved.bCritical;
	CriticalEvent.CriticalMultiplier = CriticalResolved.CriticalMultiplier;
	TestTrue(TEXT("Crit damage event preserves bCritical"), CriticalEvent.bCritical);
	TestEqual(TEXT("Crit damage event carries CriticalMultiplier"), CriticalEvent.CriticalMultiplier, 1.5f);

	FReEchoDamageEvent OrdinaryEvent;
	TestFalse(TEXT("Non-crit damage event is not critical"), OrdinaryEvent.bCritical);
	TestEqual(TEXT("Non-crit damage event defaults CriticalMultiplier to 1.0"), OrdinaryEvent.CriticalMultiplier, 1.0f);

	// 暴击跳字字号随倍率缩放：基准 52 × (1 + 暴击高出比例)。
	const float BaseWorldSize = 52.0f;
	TestEqual(TEXT("Crit damage number world size scales with multiplier"),
	          BaseWorldSize * CriticalEvent.CriticalMultiplier,
	          BaseWorldSize * 1.5f);
	return true;
}

#endif
