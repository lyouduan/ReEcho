#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"

#include "Core/ReEchoRabbitProjectilePattern.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

FString FReEchoCombatVfxCatalog::ResolvePath(const EReEchoCombatVfxSemantic Semantic)
{
	auto ResolveWeaponSlot =
	    [](const FName VisualKey, const FReEchoWeaponVfxSlot UReEchoWeaponPresentationProfile::* SlotMember)
	{
		const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(VisualKey);
		if (!Profile)
		{
			return FString();
		}
		const FReEchoWeaponVfxSlot& Slot = Profile->*SlotMember;
		return Slot.IsConfigured() ? Slot.System.ToSoftObjectPath().ToString() : FString();
	};
	switch (Semantic)
	{
		case EReEchoCombatVfxSemantic::RabbitCharging:
			return TEXT("/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Charging_01.NS_Rabbit_Charging_01");
		case EReEchoCombatVfxSemantic::RabbitProjectile:
			return TEXT("/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_Attack_02.NS_Rabbit_Attack_02");
		case EReEchoCombatVfxSemantic::PlayerHurt:
			return TEXT("/Game/VFX/Monster/Rabbit/Particle/NS_Rabbit_BeAttacked_01.NS_Rabbit_BeAttacked_01");
		case EReEchoCombatVfxSemantic::FoxCharging:
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_02.NS_Fox_Rush_02");
		case EReEchoCombatVfxSemantic::FoxDirection:
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_01.NS_Fox_Rush_01");
		case EReEchoCombatVfxSemantic::FoxDash:
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_04.NS_Fox_Rush_04");
		case EReEchoCombatVfxSemantic::PlayerMeleeSlash:
			return ResolveWeaponSlot(TEXT("CrescentBlade"), &UReEchoWeaponPresentationProfile::AttackCommitted);
		case EReEchoCombatVfxSemantic::PlayerScytheSlash:
			return ResolveWeaponSlot(TEXT("Scythe"), &UReEchoWeaponPresentationProfile::AttackCommitted);
		case EReEchoCombatVfxSemantic::PlayerBowFlight:
			return ResolveWeaponSlot(TEXT("Bow"), &UReEchoWeaponPresentationProfile::Travel);
		case EReEchoCombatVfxSemantic::PlayerBowImpact:
			return ResolveWeaponSlot(TEXT("Bow"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::PlayerGunFlight:
			return ResolveWeaponSlot(TEXT("Gun"), &UReEchoWeaponPresentationProfile::Travel);
		case EReEchoCombatVfxSemantic::PlayerGunImpact:
			return ResolveWeaponSlot(TEXT("Gun"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::EnemyHurt:
			return TEXT("/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01.NS_Rabbit_BeAttacked_01");
		default:
			return TEXT("");
	}
}

const TCHAR* FReEchoCombatVfxCatalog::ResolveRabbitProjectileTexturePath()
{
	return TEXT("/Game/VFX/Monster/Rabbit/Tex/0814_04.0814_04");
}

const TCHAR* FReEchoCombatVfxCatalog::ResolveRabbitProjectileMaterialPath()
{
	return TEXT("/Game/VFX/Monster/Rabbit/MI/BaseVFX003_Inst12.BaseVFX003_Inst12");
}

int32 FReEchoCombatVfxCatalog::GetRabbitProjectileGlowMaterialCount()
{
	return 1;
}

const TCHAR* FReEchoCombatVfxCatalog::ResolveRabbitProjectileGlowMaterialPath(const int32 LayerIndex)
{
	static const TCHAR* Paths[] = {
	    TEXT("/Game/ReEcho/Materials/VFX/M_RabbitProjectileGlow.M_RabbitProjectileGlow"),
	};
	return LayerIndex >= 0 && LayerIndex < UE_ARRAY_COUNT(Paths) ? Paths[LayerIndex] : TEXT("");
}

bool FReEchoCombatVfxCatalog::IsMeleeAttackPattern(const FName AttackPatternId)
{
	EReEchoCombatVfxSemantic Semantic = EReEchoCombatVfxSemantic::PlayerMeleeSlash;
	return ResolveMeleeAttackSemantic(AttackPatternId, Semantic);
}

bool FReEchoCombatVfxCatalog::ResolveMeleeAttackSemantic(const FName AttackPatternId,
                                                         EReEchoCombatVfxSemantic& OutSemantic)
{
	if (AttackPatternId == TEXT("Pattern.LongSwordCombo"))
	{
		OutSemantic = EReEchoCombatVfxSemantic::PlayerMeleeSlash;
		return true;
	}
	if (AttackPatternId == TEXT("Pattern.ScytheSweep"))
	{
		OutSemantic = EReEchoCombatVfxSemantic::PlayerScytheSlash;
		return true;
	}
	return false;
}

void FReEchoCombatVfxCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	for (uint8 SemanticValue = 0; SemanticValue <= static_cast<uint8>(EReEchoCombatVfxSemantic::EnemyHurt);
	     ++SemanticValue)
	{
		OutPaths.Add(ResolvePath(static_cast<EReEchoCombatVfxSemantic>(SemanticValue)));
	}
	OutPaths.Add(ResolveRabbitProjectileTexturePath());
	OutPaths.Add(ResolveRabbitProjectileMaterialPath());
	for (int32 LayerIndex = 0; LayerIndex < GetRabbitProjectileGlowMaterialCount(); ++LayerIndex)
	{
		OutPaths.Add(ResolveRabbitProjectileGlowMaterialPath(LayerIndex));
	}
}

FVector FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(const EReEchoCombatVfxSemantic Semantic)
{
	if (Semantic == EReEchoCombatVfxSemantic::RabbitProjectile)
	{
		// Runtime particle readback shows the three authored launch angles are approximately 0, 32.5 and 65
		// degrees. The middle projectile is therefore the visual center axis; the asset documentation's +Y
		// statement does not match the delivered Niagara system.
		const float CenterRadians = FMath::DegreesToRadians(ReEchoRabbitProjectilePattern::HalfSpreadDegrees);
		return FVector(FMath::Cos(CenterRadians), FMath::Sin(CenterRadians), 0.0f).GetSafeNormal();
	}
	return FVector::ForwardVector;
}

FRotator FReEchoCombatVfxCatalog::ResolveRotation(const EReEchoCombatVfxSemantic Semantic, const FVector& Direction)
{
	const FVector SafeDirection = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal2D();
	const float AuthoredYaw = ResolveAuthoredForwardAxis(Semantic).Rotation().Yaw;
	FRotator Rotation = SafeDirection.Rotation();
	Rotation.Yaw -= AuthoredYaw;
	return Rotation;
}
