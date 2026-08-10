#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/ReEchoCombatAttributeSet.h"
#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "AbilitySystem/ReEchoGameplayTags.h"
#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystemComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
struct FReEchoGasFixture
{
	UWorld* World = nullptr;
	AActor* Owner = nullptr;
	UAbilitySystemComponent* AbilitySystem = nullptr;
	UReEchoCombatAttributeSet* Attributes = nullptr;
	UReEchoCombatantComponent* Combatant = nullptr;

	FReEchoGasFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoGasTestWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		Owner = World->SpawnActor<AActor>();
		AbilitySystem = NewObject<UAbilitySystemComponent>(Owner, TEXT("TestAbilitySystem"));
		AbilitySystem->RegisterComponent();
		Attributes = NewObject<UReEchoCombatAttributeSet>(Owner, TEXT("TestAttributes"));
		AbilitySystem->AddAttributeSetSubobject(Attributes);
		AbilitySystem->InitAbilityActorInfo(Owner, Owner);
		Combatant = NewObject<UReEchoCombatantComponent>(Owner, TEXT("TestCombatant"));
		Combatant->BindToAbilitySystem(AbilitySystem);
	}

	~FReEchoGasFixture()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGasAttributeEffectTest,
                                 "ReEcho.GAS.AttributesAndEffects",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoGasAttributeEffectTest::RunTest(const FString& Parameters)
{
	FReEchoGasFixture Fixture;
	FReEchoStatBlock Stats;
	Stats.HpMax = 120.0f;
	Stats.Block = 1;
	Stats.PhysicalAttack = 17.0f;
	Stats.ElementalAttack = 13.0f;
	Stats.AttackSpeed = 1.5f;
	Stats.MovementSpeed = 1.2f;
	Stats.EchoEfficiency = 0.75f;
	TestTrue(TEXT("Initialization effect applies"),
	         ReEchoGameplayEffects::ApplyInitialization(*Fixture.AbilitySystem, Stats, true));
	TestEqual(TEXT("Health initializes from max"), Fixture.Attributes->GetHealth(), 120.0f);
	TestEqual(TEXT("Physical attack initializes"), Fixture.Attributes->GetPhysicalAttack(), 17.0f);
	TestEqual(TEXT("Movement speed initializes"), Fixture.Attributes->GetMovementSpeed(), 1.2f);

	TestEqual(TEXT("Block absorbs first damage"),
	          ReEchoGameplayEffects::ApplyDamage(nullptr, *Fixture.AbilitySystem, 25.0f),
	          0.0f);
	TestEqual(TEXT("Block is consumed"), Fixture.Attributes->GetBlock(), 0.0f);
	TestEqual(TEXT("Second hit removes health"),
	          ReEchoGameplayEffects::ApplyDamage(nullptr, *Fixture.AbilitySystem, 25.0f),
	          25.0f);
	TestEqual(TEXT("Health reflects damage"), Fixture.Attributes->GetHealth(), 95.0f);
	TestEqual(TEXT("Healing reports actual amount"),
	          ReEchoGameplayEffects::ApplyHealing(nullptr, *Fixture.AbilitySystem, 100.0f),
	          25.0f);
	TestEqual(TEXT("Healing clamps to maximum"), Fixture.Attributes->GetHealth(), 120.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGasDeathAndTagsTest,
                                 "ReEcho.GAS.DeathAndAbilityTags",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoGasDeathAndTagsTest::RunTest(const FString& Parameters)
{
	FReEchoGasFixture Fixture;
	FReEchoStatBlock Stats;
	Stats.HpMax = 40.0f;
	Fixture.Combatant->InitializeFromStats(Stats, true);
	TestFalse(TEXT("Living target has no dead tag"),
	          Fixture.AbilitySystem->HasMatchingGameplayTag(ReEchoGameplayTags::State_Dead));
	Fixture.Combatant->ApplyFinalDamage(100.0f);
	TestTrue(TEXT("Lethal effect grants dead tag"),
	         Fixture.AbilitySystem->HasMatchingGameplayTag(ReEchoGameplayTags::State_Dead));
	Fixture.Combatant->ApplyFinalDamage(100.0f);
	TestEqual(TEXT("Repeated lethal calls do not stack dead state"),
	          Fixture.AbilitySystem->GetTagCount(ReEchoGameplayTags::State_Dead),
	          1);

	Fixture.Combatant->InitializeFromStats(Stats, true);
	FGameplayAbilitySpec BasicSpec(UReEchoBasicAttackAbility::StaticClass(), 1);
	BasicSpec.GetDynamicSpecSourceTags().AddTag(ReEchoGameplayTags::Input_Attack_Basic);
	const FGameplayAbilitySpecHandle BasicHandle = Fixture.AbilitySystem->GiveAbility(BasicSpec);
	const FGameplayAbilitySpec* GrantedBasic = Fixture.AbilitySystem->FindAbilitySpecFromHandle(BasicHandle);
	TestTrue(TEXT("Input tag is stored on granted spec"),
	         GrantedBasic &&
	             GrantedBasic->GetDynamicSpecSourceTags().HasTagExact(ReEchoGameplayTags::Input_Attack_Basic));

	FGameplayEffectSpecHandle CooldownSpec = Fixture.AbilitySystem->MakeOutgoingSpec(
	    UReEchoBasicAttackCooldownEffect::StaticClass(), 1.0f, Fixture.AbilitySystem->MakeEffectContext());
	CooldownSpec.Data->SetSetByCallerMagnitude(ReEchoGameplayTags::Data_Cooldown, 1.0f);
	Fixture.AbilitySystem->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get());
	TestTrue(TEXT("Cooldown effect grants basic cooldown tag"),
	         Fixture.AbilitySystem->HasMatchingGameplayTag(ReEchoGameplayTags::Cooldown_Attack_Basic));
	TestFalse(TEXT("Cooldown rejects basic activation"), Fixture.AbilitySystem->TryActivateAbility(BasicHandle));

	FGameplayAbilitySpec ActiveSpec(UReEchoActiveAttackAbility::StaticClass(), 1);
	ActiveSpec.GetDynamicSpecSourceTags().AddTag(ReEchoGameplayTags::Input_Attack_Active);
	const FGameplayAbilitySpecHandle ActiveHandle = Fixture.AbilitySystem->GiveAbility(ActiveSpec);
	Fixture.AbilitySystem->AddLooseGameplayTag(ReEchoGameplayTags::State_Menu);
	TestFalse(TEXT("Menu state rejects active attack ability"),
	          Fixture.AbilitySystem->TryActivateAbility(ActiveHandle));
	Fixture.AbilitySystem->RemoveLooseGameplayTag(ReEchoGameplayTags::State_Menu);
	Fixture.AbilitySystem->RemoveActiveEffectsWithGrantedTags(
	    FGameplayTagContainer(ReEchoGameplayTags::Cooldown_Attack_Basic));
	Fixture.AbilitySystem->TryActivateAbility(BasicHandle);
	GrantedBasic = Fixture.AbilitySystem->FindAbilitySpecFromHandle(BasicHandle);
	TestTrue(TEXT("Failed execution ends ability and cleans active state"), GrantedBasic && !GrantedBasic->IsActive());

	const UReEchoBasicAttackAbility* Basic = GetDefault<UReEchoBasicAttackAbility>();
	const UReEchoActiveAttackAbility* Active = GetDefault<UReEchoActiveAttackAbility>();
	TestTrue(TEXT("Basic attack has ability tag"),
	         Basic->GetAssetTags().HasTagExact(ReEchoGameplayTags::Ability_Attack_Basic));
	TestTrue(TEXT("Active attack has ability tag"),
	         Active->GetAssetTags().HasTagExact(ReEchoGameplayTags::Ability_Attack_Active));
	return true;
}

#endif
