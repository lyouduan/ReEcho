#include "AbilitySystem/ReEchoGameplayTags.h"

namespace ReEchoGameplayTags
{
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Basic, "Ability.Attack.Basic", "Basic attack ability");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Active, "Ability.Attack.Active", "Active attack ability");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Weapon_Switch, "Ability.Weapon.Switch", "Weapon switch ability");
UE_DEFINE_GAMEPLAY_TAG(Input_Attack_Basic, "Input.Attack.Basic");
UE_DEFINE_GAMEPLAY_TAG(Input_Attack_Active, "Input.Attack.Active");
UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_1, "Input.Weapon.1");
UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_2, "Input.Weapon.2");
UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_3, "Input.Weapon.3");
UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
UE_DEFINE_GAMEPLAY_TAG(State_Attacking, "State.Attacking");
UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned");
UE_DEFINE_GAMEPLAY_TAG(State_Menu, "State.Menu");
UE_DEFINE_GAMEPLAY_TAG(Cooldown_Attack_Basic, "Cooldown.Attack.Basic");
UE_DEFINE_GAMEPLAY_TAG(Cooldown_Attack_Active, "Cooldown.Attack.Active");
UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Physical, "Damage.Type.Physical");
UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Elemental, "Damage.Type.Elemental");
UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");
UE_DEFINE_GAMEPLAY_TAG(Data_Heal, "Data.Heal");
UE_DEFINE_GAMEPLAY_TAG(Data_Cooldown, "Data.Cooldown");
UE_DEFINE_GAMEPLAY_TAG(Data_Health, "Data.Health");
UE_DEFINE_GAMEPLAY_TAG(Data_MaxHealth, "Data.MaxHealth");
UE_DEFINE_GAMEPLAY_TAG(Data_Block, "Data.Block");
UE_DEFINE_GAMEPLAY_TAG(Data_PhysicalAttack, "Data.PhysicalAttack");
UE_DEFINE_GAMEPLAY_TAG(Data_ElementalAttack, "Data.ElementalAttack");
UE_DEFINE_GAMEPLAY_TAG(Data_AttackSpeed, "Data.AttackSpeed");
UE_DEFINE_GAMEPLAY_TAG(Data_MovementSpeed, "Data.MovementSpeed");
UE_DEFINE_GAMEPLAY_TAG(Data_EchoEfficiency, "Data.EchoEfficiency");
}