#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"

#include "Core/ReEchoRabbitProjectilePattern.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

namespace
{
const FReEchoWeaponVfxSlot* ResolveWeaponSlot(const EReEchoCombatVfxSemantic Semantic)
{
	FName VisualKey = NAME_None;
	const FReEchoWeaponVfxSlot UReEchoWeaponPresentationProfile::* SlotMember = nullptr;
	switch (Semantic)
	{
		case EReEchoCombatVfxSemantic::PlayerMeleeSlash:
			VisualKey = TEXT("CrescentBlade");
			SlotMember = &UReEchoWeaponPresentationProfile::AttackCommitted;
			break;
		case EReEchoCombatVfxSemantic::PlayerScytheSlash:
			VisualKey = TEXT("Scythe");
			SlotMember = &UReEchoWeaponPresentationProfile::AttackCommitted;
			break;
		case EReEchoCombatVfxSemantic::PlayerLongSwordImpact:
			VisualKey = TEXT("CrescentBlade");
			SlotMember = &UReEchoWeaponPresentationProfile::DamageApplied;
			break;
		case EReEchoCombatVfxSemantic::PlayerScytheImpact:
			VisualKey = TEXT("Scythe");
			SlotMember = &UReEchoWeaponPresentationProfile::DamageApplied;
			break;
		case EReEchoCombatVfxSemantic::PlayerBowFlight:
			VisualKey = TEXT("Bow");
			SlotMember = &UReEchoWeaponPresentationProfile::Travel;
			break;
		case EReEchoCombatVfxSemantic::PlayerBowImpact:
			// Bow and Gun intentionally share the normal bullet-hit effect.
			VisualKey = TEXT("Gun");
			SlotMember = &UReEchoWeaponPresentationProfile::DamageApplied;
			break;
		case EReEchoCombatVfxSemantic::PlayerGunFlight:
			VisualKey = TEXT("Gun");
			SlotMember = &UReEchoWeaponPresentationProfile::Travel;
			break;
		case EReEchoCombatVfxSemantic::PlayerGunImpact:
			VisualKey = TEXT("Gun");
			SlotMember = &UReEchoWeaponPresentationProfile::DamageApplied;
			break;
		case EReEchoCombatVfxSemantic::PlayerProjectileExplosionImpact:
			// The delivered Bow boom is the shared ranged explosion impact asset.
			VisualKey = TEXT("Bow");
			SlotMember = &UReEchoWeaponPresentationProfile::DamageApplied;
			break;
		default:
			return nullptr;
	}
	const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(VisualKey);
	return Profile && SlotMember ? &(Profile->*SlotMember) : nullptr;
}
}

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
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Charging.NS_Fox_Rush_Charging");
		case EReEchoCombatVfxSemantic::FoxDirection:
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_arrow.NS_Fox_Rush_arrow");
		case EReEchoCombatVfxSemantic::FoxDash:
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_Trail.NS_Fox_Rush_Trail");
		case EReEchoCombatVfxSemantic::FoxImpact:
			return TEXT("/Game/VFX/Monster/Fox/Particle/NS_Fox_Rush_BeAttacked.NS_Fox_Rush_BeAttacked");
		case EReEchoCombatVfxSemantic::PlayerMeleeSlash:
			return ResolveWeaponSlot(TEXT("CrescentBlade"), &UReEchoWeaponPresentationProfile::AttackCommitted);
		case EReEchoCombatVfxSemantic::PlayerScytheSlash:
			return ResolveWeaponSlot(TEXT("Scythe"), &UReEchoWeaponPresentationProfile::AttackCommitted);
		case EReEchoCombatVfxSemantic::PlayerLongSwordImpact:
			return ResolveWeaponSlot(TEXT("CrescentBlade"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::PlayerScytheImpact:
			return ResolveWeaponSlot(TEXT("Scythe"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::PlayerBowFlight:
			return ResolveWeaponSlot(TEXT("Bow"), &UReEchoWeaponPresentationProfile::Travel);
		case EReEchoCombatVfxSemantic::PlayerBowImpact:
			return ResolveWeaponSlot(TEXT("Gun"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::PlayerGunFlight:
			return ResolveWeaponSlot(TEXT("Gun"), &UReEchoWeaponPresentationProfile::Travel);
		case EReEchoCombatVfxSemantic::PlayerGunImpact:
			return ResolveWeaponSlot(TEXT("Gun"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::PlayerProjectileExplosionImpact:
			return ResolveWeaponSlot(TEXT("Bow"), &UReEchoWeaponPresentationProfile::DamageApplied);
		case EReEchoCombatVfxSemantic::EnemyHurt:
			return TEXT("/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01.NS_Rabbit_BeAttacked_01");
		case EReEchoCombatVfxSemantic::EchoWaterAura:
			return TEXT("/Game/VFX/Echo/Particle/NS_Echo_Water.NS_Echo_Water");
		case EReEchoCombatVfxSemantic::EchoGrassAura:
			return TEXT("/Game/VFX/Echo/Particle/NS_Echo_Grass.NS_Echo_Grass");
		case EReEchoCombatVfxSemantic::GoatSkill02Charging:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Charging.NS_Goat_Skill02_Charging");
		case EReEchoCombatVfxSemantic::GoatSkill02Bullet:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_Bullet.NS_Goat_Skill02_Bullet");
		case EReEchoCombatVfxSemantic::GoatSkill02Impact:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill02_BeAttacked.NS_Goat_Skill02_BeAttacked");
		case EReEchoCombatVfxSemantic::GoatSkill03Charging:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Charging.NS_Goat_Skill03_Charging");
		case EReEchoCombatVfxSemantic::GoatSkill03Alarming:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_Alarming.NS_Goat_Skill03_Alarming");
		case EReEchoCombatVfxSemantic::GoatSkill03Impact:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill03_BeAttacked.NS_Goat_Skill03_BeAttacked");
		case EReEchoCombatVfxSemantic::GoatSkill04Charging:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Charging.NS_Goat_Skill04_Charging");
		case EReEchoCombatVfxSemantic::GoatSkill04Lighting:
			return TEXT("/Game/VFX/Monster/Goat/Particle/NS_Goat_Skill04_Lighting.NS_Goat_Skill04_Lighting");
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

bool FReEchoCombatVfxCatalog::ResolveWeaponDamageSemantic(const FName WeaponId, EReEchoCombatVfxSemantic& OutSemantic)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvWeaponRow* Weapon = Snapshot.IsValid() ? Snapshot->FindEnabledWeapon(WeaponId) : nullptr;
	if (!Weapon)
	{
		return false;
	}
	if (Weapon->VisualKey == TEXT("CrescentBlade"))
	{
		OutSemantic = EReEchoCombatVfxSemantic::PlayerLongSwordImpact;
		return true;
	}
	if (Weapon->VisualKey == TEXT("Scythe"))
	{
		OutSemantic = EReEchoCombatVfxSemantic::PlayerScytheImpact;
		return true;
	}
	return false;
}

bool FReEchoCombatVfxCatalog::ResolveProjectileImpactSemantic(const FName WeaponVisualKey,
                                                              const float ExplosionRadiusCm,
                                                              EReEchoCombatVfxSemantic& OutSemantic)
{
	if (WeaponVisualKey != TEXT("Bow") && WeaponVisualKey != TEXT("Gun"))
	{
		return false;
	}
	OutSemantic = ExplosionRadiusCm > 0.0f ? EReEchoCombatVfxSemantic::PlayerProjectileExplosionImpact
	                                       : EReEchoCombatVfxSemantic::PlayerGunImpact;
	return true;
}

float FReEchoCombatVfxCatalog::ResolveMeleeSlashDelay(const EReEchoCombatVfxSemantic Semantic)
{
	if (Semantic == EReEchoCombatVfxSemantic::PlayerMeleeSlash)
	{
		// Longsword is a forward 180-degree slash and releases its VFX at commit time.
		return 0.0f;
	}
	if (Semantic == EReEchoCombatVfxSemantic::PlayerScytheSlash)
	{
		const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(TEXT("Scythe"));
		return Profile ? FMath::Max(Profile->MotionDurationSeconds, 0.0f) : 0.0f;
	}
	return 0.0f;
}

void FReEchoCombatVfxCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	for (uint8 SemanticValue = 0; SemanticValue <= static_cast<uint8>(EReEchoCombatVfxSemantic::GoatSkill04Lighting);
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
	if (Semantic == EReEchoCombatVfxSemantic::PlayerBowFlight)
	{
		// Side-on PIE confirmation identifies the delivered arrowhead's authored visual axis as local +Y.
		return FVector::RightVector;
	}
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

FReEchoVfxPlacement FReEchoCombatVfxCatalog::ResolvePlacement(const EReEchoCombatVfxSemantic Semantic)
{
	FReEchoVfxPlacement Placement;
	if (const FReEchoWeaponVfxSlot* Slot = ::ResolveWeaponSlot(Semantic))
	{
		Placement.LocalOffset = Slot->Offset.GetTranslation();
		Placement.LocalRotation = Slot->Offset.GetRotation().Rotator();
		Placement.Scale = Slot->Offset.GetScale3D();
		Placement.ScalePolicy = Slot->bPreserveWorldSize ? EReEchoVfxScalePolicy::PreserveWorldSize
		                                                 : EReEchoVfxScalePolicy::InheritAttachment;
		Placement.bUseWorldDirectionRotation = true;
		Placement.PlaybackDurationSeconds = FMath::Max(Slot->PlaybackDurationSeconds, 0.01f);
		return Placement;
	}
	if (Semantic == EReEchoCombatVfxSemantic::GoatSkill02Charging ||
	    Semantic == EReEchoCombatVfxSemantic::GoatSkill03Charging ||
	    Semantic == EReEchoCombatVfxSemantic::GoatSkill04Charging)
	{
		// These systems describe a world-sized body charge. Their anchor follows the Boss, but its authored size
		// must not be multiplied a second time by DA/Actor presentation scale.
		Placement.ScalePolicy = EReEchoVfxScalePolicy::PreserveWorldSize;
	}
	return Placement;
}
