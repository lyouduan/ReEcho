#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"

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
	return Pattern.Contains(TEXT("LongSword")) || Pattern.Contains(TEXT("Dagger")) || Pattern.Contains(TEXT("Scythe"));
}

FRotator FReEchoCombatVfxCatalog::ResolveRotation(const FVector& Direction, const bool bLocalYAxisForward)
{
	const FVector SafeDirection = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal2D();
	FRotator Rotation = SafeDirection.Rotation();
	if (bLocalYAxisForward)
	{
		Rotation.Yaw -= 90.0f;
	}
	return Rotation;
}
