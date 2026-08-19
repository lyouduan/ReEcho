#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"

#include "Core/ReEchoRabbitProjectilePattern.h"

const TCHAR* FReEchoCombatVfxCatalog::ResolvePath(const EReEchoCombatVfxSemantic Semantic)
{
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
			return TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01.NS_People_Sword_Attack_01");
		case EReEchoCombatVfxSemantic::EnemyHurt:
			return TEXT("/Game/VFX/People/Sword/Particle/NS_Rabbit_BeAttacked_01.NS_Rabbit_BeAttacked_01");
		default:
			return TEXT("");
	}
}

bool FReEchoCombatVfxCatalog::IsMeleeAttackPattern(const FName AttackPatternId)
{
	const FString Pattern = AttackPatternId.ToString();
	return Pattern.Contains(TEXT("LongSword")) || Pattern.Contains(TEXT("Scythe"));
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
