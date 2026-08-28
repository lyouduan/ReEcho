#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#include "ReEchoAudioTypes.h"
#include "ReEchoAudioEvents.h"
#include "ReEchoAudioCatalog.h"
#include "ReEchoAudioBackend.h"
#include "ReEchoAudioPolicyEngine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
/** Records backend calls; used so tests need no speakers or real assets. */
class FFakeAudioBackend : public IReEchoAudioBackend
{
public:
	int32 PlayOneShotCount = 0;
	int32 StopOneShotCount = 0;
	int32 StartLoopCount = 0;
	int32 StopLoopCount = 0;
	float FakeVoiceDuration = 1.0f;
	FReEchoAudioPlayCommand LastOneShotCommand;
	FReEchoAudioPlayCommand LastLoopCommand;
	bool bFailLoopStart = false;
	TMap<uint32, float> LoopVolumes;
	uint32 NextHandle = 1;

	virtual bool IsAvailable() const override
	{
		return true;
	}

	virtual uint32 PlayOneShot(const FReEchoAudioPlayCommand& C) override
	{
		LastOneShotCommand = C;
		PlayOneShotCount++;
		return NextHandle++;
	}

	virtual void StopOneShot(uint32 VoiceHandle) override
	{
		StopOneShotCount++;
	}

	virtual uint32 StartLoop(const FReEchoAudioPlayCommand& C) override
	{
		LastLoopCommand = C;
		StartLoopCount++;
		if (bFailLoopStart)
		{
			return 0;
		}
		const uint32 H = NextHandle++;
		LoopVolumes.Add(H, C.Volume);
		return H;
	}

	virtual void StopLoop(uint32 LoopHandle, float FadeOutSeconds) override
	{
		StopLoopCount++;
		LoopVolumes.Remove(LoopHandle);
	}

	virtual void SetLoopVolume(uint32 LoopHandle, float Volume) override
	{
		LoopVolumes.FindOrAdd(LoopHandle) = Volume;
	}

	virtual float GetVoiceDuration(const FReEchoAudioPlayCommand& C) const override
	{
		return FakeVoiceDuration;
	}
};

struct FAudioTestHarness
{
	TSharedPtr<FFakeAudioBackend> Backend;
	TSharedPtr<FReEchoAudioCatalog> Catalog;
	TSharedPtr<FReEchoAudioPolicyEngine> Engine;

	void Setup()
	{
		Backend = MakeShared<FFakeAudioBackend>();
		Catalog = MakeShared<FReEchoAudioCatalog>();
		Engine = MakeShared<FReEchoAudioPolicyEngine>();
		Engine->SetCatalogProvider(Catalog);
		Engine->SetBackend(Backend);
	}

	FReEchoAudioEventDefinition MakeDef(FName Id, EReEchoAudioBus Bus, bool bLoop)
	{
		FReEchoAudioEventDefinition Def;
		Def.EventId = Id;
		Def.Bus = Bus;
		Def.Type = bLoop ? EReEchoAudioEventType::Loop : EReEchoAudioEventType::OneShot;
		return Def;
	}

	void AddDef(const FReEchoAudioEventDefinition& Def)
	{
		Catalog->AddDefinition(Def);
	}
};

FReEchoAudioEventRequest Req(FName Id)
{
	FReEchoAudioEventRequest R;
	R.EventId = Id;
	return R;
}
}

// ---- Pause policy reaches both one-shot and state playback commands ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioPausePolicyTest,
                                 "ReEcho.Audio.Foundation.PausePolicy",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioPausePolicyTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	FReEchoAudioEventDefinition OneShot = H.MakeDef(FReEchoAudioEvents::UiConfirm, EReEchoAudioBus::UiSfx, false);
	OneShot.PausePolicy = EReEchoAudioPausePolicy::ContinueOnPause;
	OneShot.bSpatial3D = true;
	OneShot.AttenuationMin = 200.0f;
	OneShot.AttenuationMax = 2000.0f;
	OneShot.StartTimeSeconds = 0.25f;
	H.AddDef(OneShot);
	FReEchoAudioEventRequest OneShotRequest = Req(FReEchoAudioEvents::UiConfirm);
	OneShotRequest.WorldLocation = FVector(10.0f, 20.0f, 30.0f);
	H.Engine->PostEvent(OneShotRequest);
	TestEqual(TEXT("one-shot preserves continue-on-pause policy"),
	          H.Backend->LastOneShotCommand.PausePolicy,
	          EReEchoAudioPausePolicy::ContinueOnPause);
	TestTrue(TEXT("one-shot preserves spatialization"), H.Backend->LastOneShotCommand.bSpatial3D);
	TestEqual(
	    TEXT("one-shot preserves location"), H.Backend->LastOneShotCommand.Location, OneShotRequest.WorldLocation);
	TestEqual(TEXT("one-shot preserves attenuation minimum"), H.Backend->LastOneShotCommand.AttenuationMin, 200.0f);
	TestEqual(TEXT("one-shot preserves attenuation maximum"), H.Backend->LastOneShotCommand.AttenuationMax, 2000.0f);
	TestEqual(TEXT("one-shot preserves configured start time"), H.Backend->LastOneShotCommand.StartTimeSeconds, 0.25f);

	FReEchoAudioEventDefinition Loop = H.MakeDef(FReEchoAudioEvents::MusicMenu, EReEchoAudioBus::Music, true);
	Loop.PausePolicy = EReEchoAudioPausePolicy::ContinueOnPause;
	Loop.bSpatial3D = true;
	Loop.AttenuationMin = 300.0f;
	Loop.AttenuationMax = 2400.0f;
	Loop.StartTimeSeconds = 12.5f;
	H.AddDef(Loop);
	UWorld* ExpectedWorld = reinterpret_cast<UWorld*>(UPTRINT(1));
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicMenu, ExpectedWorld);
	TestEqual(TEXT("state preserves continue-on-pause policy"),
	          H.Backend->LastLoopCommand.PausePolicy,
	          EReEchoAudioPausePolicy::ContinueOnPause);
	TestEqual(TEXT("state forwards its world"), H.Backend->LastLoopCommand.World, ExpectedWorld);
	TestTrue(TEXT("state requests a fade-in"), H.Backend->LastLoopCommand.FadeInSeconds > 0.0f);
	TestTrue(TEXT("state preserves spatialization"), H.Backend->LastLoopCommand.bSpatial3D);
	TestEqual(TEXT("state preserves attenuation minimum"), H.Backend->LastLoopCommand.AttenuationMin, 300.0f);
	TestEqual(TEXT("state preserves attenuation maximum"), H.Backend->LastLoopCommand.AttenuationMax, 2400.0f);
	TestEqual(TEXT("state preserves configured start time"), H.Backend->LastLoopCommand.StartTimeSeconds, 12.5f);
	return true;
}

// ---- Unknown event is safely rejected, and warned at most once ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioUnknownEventTest,
                                 "ReEcho.Audio.Foundation.UnknownEventRejected",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioUnknownEventTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	const FName Unknown = FName(TEXT("Does.Not.Exist"));
	H.Engine->PostEvent(Req(Unknown));
	H.Engine->PostEvent(Req(Unknown));
	H.Engine->PostEvent(Req(Unknown));

	TestEqual(TEXT("no play for unknown event"), H.Backend->PlayOneShotCount, 0);
	TestEqual(TEXT("unknown event warned exactly once"), H.Engine->GetUnknownEventWarningCount(), 1);
	return true;
}

// ---- Master * bus * event volume, with clamp ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioVolumeMathTest,
                                 "ReEcho.Audio.Foundation.VolumeMath",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioVolumeMathTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	TestEqual(TEXT("default volume is 1.0"), H.Engine->ComputeOneShotVolume(EReEchoAudioBus::CombatSfx, 1.0f), 1.0f);
	TestEqual(TEXT("volume clamped to 1.0"), H.Engine->ComputeOneShotVolume(EReEchoAudioBus::CombatSfx, 2.0f), 1.0f);

	H.Engine->SetMasterVolume(0.5f);
	H.Engine->SetBusVolume(EReEchoAudioBus::CombatSfx, 0.5f);
	// 0.5 * 0.5 * 0.5 = 0.125
	TestEqual(
	    TEXT("master * bus * event base"), H.Engine->ComputeOneShotVolume(EReEchoAudioBus::CombatSfx, 0.5f), 0.125f);
	return true;
}

// ---- Mute (bus and master) forces zero volume ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioMuteTest,
                                 "ReEcho.Audio.Foundation.Mute",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioMuteTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	H.Engine->SetBusMuted(EReEchoAudioBus::CombatSfx, true);
	TestEqual(TEXT("muted bus => 0"), H.Engine->ComputeOneShotVolume(EReEchoAudioBus::CombatSfx, 1.0f), 0.0f);

	H.Engine->SetBusMuted(EReEchoAudioBus::CombatSfx, false);
	H.Engine->SetBusMuted(EReEchoAudioBus::Master, true);
	TestEqual(TEXT("muted master => 0"), H.Engine->ComputeOneShotVolume(EReEchoAudioBus::CombatSfx, 1.0f), 0.0f);
	return true;
}

// ---- Cooldown drops repeated plays within the window ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioCooldownTest,
                                 "ReEcho.Audio.Foundation.Cooldown",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioCooldownTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	FReEchoAudioEventDefinition Def = H.MakeDef(FReEchoAudioEvents::CombatAttack, EReEchoAudioBus::CombatSfx, false);
	Def.CooldownSeconds = 1.0f;
	Def.MaxConcurrency = 0;
	H.AddDef(Def);

	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	TestEqual(TEXT("second play within cooldown dropped"), H.Backend->PlayOneShotCount, 1);

	H.Engine->Update(1.1f);
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	TestEqual(TEXT("play allowed after cooldown"), H.Backend->PlayOneShotCount, 2);
	return true;
}

// ---- Per-event concurrency limit drops extra plays ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioConcurrencyTest,
                                 "ReEcho.Audio.Foundation.Concurrency",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioConcurrencyTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	FReEchoAudioEventDefinition Def = H.MakeDef(FReEchoAudioEvents::CombatAttack, EReEchoAudioBus::CombatSfx, false);
	Def.MaxConcurrency = 2;
	Def.Priority = 1;
	H.AddDef(Def);

	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));

	TestEqual(TEXT("only two voices played"), H.Backend->PlayOneShotCount, 2);
	TestEqual(TEXT("two active voices tracked"), H.Engine->GetActiveVoiceCount(FReEchoAudioEvents::CombatAttack), 2);
	return true;
}

// ---- Priority: higher priority preempts lower on the same bus;
//       equal/lower at its own limit is dropped ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioPriorityTest,
                                 "ReEcho.Audio.Foundation.Priority",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioPriorityTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	// Low-priority filler on CombatSfx bus (high own limit so it never self-limits).
	FReEchoAudioEventDefinition Low = H.MakeDef(FReEchoAudioEvents::EnemyAttack, EReEchoAudioBus::CombatSfx, false);
	Low.MaxConcurrency = 10;
	Low.Priority = 0;
	H.AddDef(Low);

	// High-priority event with a tight own limit.
	FReEchoAudioEventDefinition High = H.MakeDef(FReEchoAudioEvents::CombatAttack, EReEchoAudioBus::CombatSfx, false);
	High.MaxConcurrency = 2;
	High.Priority = 5;
	H.AddDef(High);

	// 3 low voices, then 2 high voices (high still under its own limit).
	H.Engine->PostEvent(Req(FReEchoAudioEvents::EnemyAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::EnemyAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::EnemyAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	TestEqual(TEXT("baseline plays"), H.Backend->PlayOneShotCount, 5);
	TestEqual(TEXT("high has 2 active"), H.Engine->GetActiveVoiceCount(FReEchoAudioEvents::CombatAttack), 2);

	// High wants a 3rd voice (at its own limit) -> preempts a low-priority voice on same bus.
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	TestEqual(TEXT("high preempts a low voice (played)"), H.Backend->PlayOneShotCount, 6);
	TestEqual(TEXT("low voice stopped"), H.Backend->StopOneShotCount, 1);
	TestEqual(TEXT("high now has 3 active"), H.Engine->GetActiveVoiceCount(FReEchoAudioEvents::CombatAttack), 3);
	TestEqual(TEXT("low reduced to 2"), H.Engine->GetActiveVoiceCount(FReEchoAudioEvents::EnemyAttack), 2);

	// Equal priority at its own limit is dropped (no preemption of equal priority).
	FReEchoAudioEventDefinition Equal = H.MakeDef(FReEchoAudioEvents::CombatHit, EReEchoAudioBus::CombatSfx, false);
	Equal.MaxConcurrency = 1;
	Equal.Priority = 0;
	H.AddDef(Equal);
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatHit)); // plays (1 active)
	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatHit)); // at own limit, equal priority -> dropped
	TestEqual(TEXT("equal priority at limit dropped"), H.Backend->PlayOneShotCount, 7);

	return true;
}

// ---- State channels are idempotent and transition exactly once ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioStateTest,
                                 "ReEcho.Audio.Foundation.StateIdempotent",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioStateTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	FReEchoAudioEventDefinition Enc = H.MakeDef(FReEchoAudioEvents::MusicEncounter, EReEchoAudioBus::Music, true);
	Enc.BaseVolume = 1.0f;
	H.AddDef(Enc);
	FReEchoAudioEventDefinition Boss = H.MakeDef(FReEchoAudioEvents::MusicBoss, EReEchoAudioBus::Music, true);
	H.AddDef(Boss);

	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicEncounter);
	TestEqual(TEXT("loop started once"), H.Backend->StartLoopCount, 1);
	TestEqual(TEXT("active loop count 1"), H.Engine->GetActiveLoopCount(), 1);
	TestEqual(TEXT("current state set"),
	          H.Engine->GetCurrentState(EReEchoAudioChannel::Music),
	          FReEchoAudioEvents::MusicEncounter);

	// Repeated same state is idempotent.
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicEncounter);
	TestEqual(TEXT("still only one loop (idempotent)"), H.Backend->StartLoopCount, 1);

	// Different state: exactly one transition (stop old, start new).
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicBoss);
	TestEqual(TEXT("old loop stopped"), H.Backend->StopLoopCount, 1);
	TestEqual(TEXT("new loop started"), H.Backend->StartLoopCount, 2);
	TestEqual(TEXT("transition happened once"), H.Engine->GetActiveLoopCount(), 1);
	TestEqual(TEXT("current state updated"),
	          H.Engine->GetCurrentState(EReEchoAudioChannel::Music),
	          FReEchoAudioEvents::MusicBoss);

	// Unknown or unplayable replacements preserve the live state.
	H.Engine->SetState(EReEchoAudioChannel::Music, FName(TEXT("Music.Unknown")));
	TestEqual(TEXT("unknown replacement preserves current state"),
	          H.Engine->GetCurrentState(EReEchoAudioChannel::Music),
	          FReEchoAudioEvents::MusicBoss);
	TestEqual(TEXT("unknown replacement does not stop current loop"), H.Backend->StopLoopCount, 1);

	FReEchoAudioEventDefinition Menu = H.MakeDef(FReEchoAudioEvents::MusicMenu, EReEchoAudioBus::Music, true);
	H.AddDef(Menu);
	H.Backend->bFailLoopStart = true;
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicMenu);
	TestEqual(TEXT("failed replacement preserves current state"),
	          H.Engine->GetCurrentState(EReEchoAudioChannel::Music),
	          FReEchoAudioEvents::MusicBoss);
	TestEqual(TEXT("failed replacement does not stop current loop"), H.Backend->StopLoopCount, 1);
	H.Backend->bFailLoopStart = false;
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicMenu);
	TestEqual(TEXT("failed replacement remains retryable"),
	          H.Engine->GetCurrentState(EReEchoAudioChannel::Music),
	          FReEchoAudioEvents::MusicMenu);
	TestEqual(TEXT("successful retry stops previous loop once"), H.Backend->StopLoopCount, 2);

	FReEchoAudioEventDefinition StageVariant = Enc;
	StageVariant.VariantId = TEXT("Stage.1");
	H.AddDef(StageVariant);
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicEncounter, StageVariant.VariantId);
	TestEqual(TEXT("variant state becomes current"),
	          H.Engine->GetCurrentState(EReEchoAudioChannel::Music),
	          FReEchoAudioEvents::MusicEncounter);
	TestEqual(TEXT("variant id becomes current"),
	          H.Engine->GetCurrentStateVariant(EReEchoAudioChannel::Music),
	          StageVariant.VariantId);
	TestEqual(TEXT("variant reaches backend command"), H.Backend->LastLoopCommand.VariantId, StageVariant.VariantId);
	const int32 StartCountAfterVariant = H.Backend->StartLoopCount;
	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicEncounter, StageVariant.VariantId);
	TestEqual(TEXT("same state and variant remain idempotent"), H.Backend->StartLoopCount, StartCountAfterVariant);
	return true;
}

// ---- StopState and full shutdown clean up all loops ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioCleanupTest,
                                 "ReEcho.Audio.Foundation.StopStateAndCleanup",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioCleanupTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	FReEchoAudioEventDefinition Enc = H.MakeDef(FReEchoAudioEvents::MusicEncounter, EReEchoAudioBus::Music, true);
	H.AddDef(Enc);
	FReEchoAudioEventDefinition Arena = H.MakeDef(FReEchoAudioEvents::AmbienceArena, EReEchoAudioBus::Ambience, true);
	H.AddDef(Arena);

	H.Engine->SetState(EReEchoAudioChannel::Music, FReEchoAudioEvents::MusicEncounter);
	H.Engine->SetState(EReEchoAudioChannel::Ambience, FReEchoAudioEvents::AmbienceArena);
	TestEqual(TEXT("two loops active"), H.Engine->GetActiveLoopCount(), 2);

	H.Engine->StopState(EReEchoAudioChannel::Music);
	TestEqual(TEXT("music loop stopped via StopState"), H.Backend->StopLoopCount, 1);
	TestEqual(TEXT("music state cleared"), H.Engine->GetCurrentState(EReEchoAudioChannel::Music), NAME_None);
	TestEqual(TEXT("one loop remains"), H.Engine->GetActiveLoopCount(), 1);

	H.Engine->Shutdown();
	TestEqual(TEXT("all loops stopped on shutdown"), H.Backend->StopLoopCount, 2);
	TestEqual(TEXT("no active loops after shutdown"), H.Engine->GetActiveLoopCount(), 0);
	return true;
}

// ---- No audio device / no asset must not crash ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioNoAssetTest,
                                 "ReEcho.Audio.Foundation.NoDeviceNoAsset",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioNoAssetTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	// Real backend with a null sound and null world must degrade to no-op.
	TSharedRef<IReEchoAudioBackend> RealBackend = ReEchoAudio::CreateUnrealBackend();
	H.Engine->SetBackend(RealBackend);

	// Event with no sound asset (TSoftObjectPtr default null).
	FReEchoAudioEventDefinition Def = H.MakeDef(FReEchoAudioEvents::CombatAttack, EReEchoAudioBus::CombatSfx, false);
	H.AddDef(Def);

	H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack)); // should not crash
	TestEqual(TEXT("no voice tracked when asset missing"), H.Engine->GetTotalActiveVoices(), 0);
	TestEqual(TEXT("no unknown warning for defined event"), H.Engine->GetUnknownEventWarningCount(), 0);

	// Unknown event also safe with real backend.
	H.Engine->PostEvent(Req(FName(TEXT("Missing.Event"))));
	TestEqual(TEXT("unknown warned once, no crash"), H.Engine->GetUnknownEventWarningCount(), 1);
	return true;
}

// ---- Audio random variation must not touch gameplay RNG ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioRandomIsolationTest,
                                 "ReEcho.Audio.Foundation.RandomIsolation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioRandomIsolationTest::RunTest(const FString& Parameters)
{
	FAudioTestHarness H;
	H.Setup();

	FReEchoAudioEventDefinition Def = H.MakeDef(FReEchoAudioEvents::CombatAttack, EReEchoAudioBus::CombatSfx, false);
	Def.MaxConcurrency = 0;
	Def.PitchMin = 0.8f;
	Def.PitchMax = 1.2f;
	H.AddDef(Def);

	// Expected gameplay-RNG draws from a known seed (no audio in between yet).
	FMath::RandInit(20240812);
	const int32 ExpectedFirst = FMath::Rand();
	const int32 ExpectedSecond = FMath::Rand();

	FMath::RandInit(20240812);
	const int32 GotFirst = FMath::Rand();
	// Spam audio events; they must use their own stream, not the global one.
	for (int32 i = 0; i < 200; ++i)
	{
		H.Engine->PostEvent(Req(FReEchoAudioEvents::CombatAttack));
	}
	const int32 GotSecond = FMath::Rand();

	TestEqual(TEXT("gameplay RNG first draw untouched"), GotFirst, ExpectedFirst);
	TestEqual(TEXT("gameplay RNG second draw untouched"), GotSecond, ExpectedSecond);
	return true;
}

// ---- Catalog reload is quote-aware and atomic ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioCatalogAtomicLoadTest,
                                 "ReEcho.Audio.Catalog.AtomicLoad",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioCatalogAtomicLoadTest::RunTest(const FString& Parameters)
{
	const FString Header = TEXT("EventId,VariantId,AssetPath,Bus,EventType,Spatial3D,BaseVolume,PitchMin,PitchMax,"
	                            "CooldownSeconds,MaxConcurrency,Priority,PausePolicy,AttenuationMin,AttenuationMax,"
	                            "StartTimeSeconds\n");
	const FString ValidPath =
	    FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("AudioCatalogValid"), TEXT(".csv"));
	const FString InvalidPath =
	    FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("AudioCatalogInvalid"), TEXT(".csv"));
	FFileHelper::SaveStringToFile(
	    Header +
	        TEXT("\"Combat.Attack\",,,CombatSfx,OneShot,true,0.8,0.9,1.1,0.05,4,20,PauseWithGame,200,2000,0.125\n"),
	    *ValidPath);
	FFileHelper::SaveStringToFile(
	    Header + TEXT("Combat.Attack,,,InvalidBus,OneShot,true,0.8,0.9,1.1,0.05,4,20,PauseWithGame,200,2000,0\n"),
	    *InvalidPath);

	FReEchoAudioCatalog Catalog;
	TestTrue(TEXT("valid quoted CSV loads"), Catalog.LoadCatalog(ValidPath));
	TestEqual(TEXT("one definition committed"), Catalog.Num(), 1);
	const FReEchoAudioEventDefinition* Before = Catalog.FindDefinition(FReEchoAudioEvents::CombatAttack);
	TestNotNull(TEXT("stable event is available"), Before);
	TestEqual(TEXT("catalog parses configured start time"), Before ? Before->StartTimeSeconds : -1.0f, 0.125f);
	AddExpectedError(TEXT("unsupported bus; preserving previous catalog"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("invalid enum rejects reload"), Catalog.LoadCatalog(InvalidPath));
	TestEqual(TEXT("failed reload preserves previous catalog"), Catalog.Num(), 1);
	TestNotNull(TEXT("previous definition remains available"),
	            Catalog.FindDefinition(FReEchoAudioEvents::CombatAttack));
	FReEchoAudioEventDefinition Variant;
	Variant.EventId = FReEchoAudioEvents::CombatAttack;
	Variant.VariantId = TEXT("W_J_01");
	Catalog.AddDefinition(Variant);
	const FReEchoAudioEventDefinition* Exact = Catalog.FindDefinition(FReEchoAudioEvents::CombatAttack, TEXT("W_J_01"));
	TestNotNull(TEXT("exact event variant resolves"), Exact);
	TestEqual(TEXT("exact event variant is returned"), Exact ? Exact->VariantId : NAME_None, FName(TEXT("W_J_01")));
	const FReEchoAudioEventDefinition* Fallback =
	    Catalog.FindDefinition(FReEchoAudioEvents::CombatAttack, TEXT("UnknownWeapon"));
	TestNotNull(TEXT("unknown variant falls back to base event"), Fallback);
	TestTrue(TEXT("fallback has no variant id"), Fallback && Fallback->VariantId.IsNone());

	IFileManager::Get().Delete(*ValidPath);
	IFileManager::Get().Delete(*InvalidPath);
	return true;
}

// ---- Module loads and stable event/state constants are valid ----

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAudioModuleLoadTest,
                                 "ReEcho.Audio.Foundation.ModuleLoads",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAudioModuleLoadTest::RunTest(const FString& Parameters)
{
	IModuleInterface* Module = FModuleManager::Get().LoadModule(TEXT("ReEchoAudio"));
	TestNotNull(TEXT("ReEchoAudio module loads"), Module);

	TestFalse(TEXT("Music.Menu constant valid"), FReEchoAudioEvents::MusicMenu.IsNone());
	TestFalse(TEXT("Ambience.Arena constant valid"), FReEchoAudioEvents::AmbienceArena.IsNone());
	TestFalse(TEXT("UI.Confirm constant valid"), FReEchoAudioEvents::UiConfirm.IsNone());
	TestFalse(TEXT("UI.CardReveal constant valid"), FReEchoAudioEvents::UiCardReveal.IsNone());
	TestFalse(TEXT("UI.Equip constant valid"), FReEchoAudioEvents::UiEquip.IsNone());
	TestFalse(TEXT("UI.Unequip constant valid"), FReEchoAudioEvents::UiUnequip.IsNone());
	TestFalse(TEXT("Combat.Attack constant valid"), FReEchoAudioEvents::CombatAttack.IsNone());
	TestFalse(TEXT("Combat.Reaction constant valid"), FReEchoAudioEvents::CombatReaction.IsNone());
	TestFalse(TEXT("Enemy.Spawn constant valid"), FReEchoAudioEvents::EnemySpawn.IsNone());
	TestFalse(TEXT("Boss.Death constant valid"), FReEchoAudioEvents::BossDeath.IsNone());
	TestFalse(TEXT("Echo.End constant valid"), FReEchoAudioEvents::EchoEnd.IsNone());
	TestFalse(TEXT("CameraMove constant valid"), FReEchoAudioEvents::CameraMove.IsNone());
	TestFalse(TEXT("Revive constant valid"), FReEchoAudioEvents::Revive.IsNone());
	TestFalse(TEXT("Item.Pickup constant valid"), FReEchoAudioEvents::ItemPickup.IsNone());
	TestFalse(TEXT("Flow.Victory constant valid"), FReEchoAudioEvents::FlowVictory.IsNone());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
