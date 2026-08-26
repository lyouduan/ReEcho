#!/usr/bin/env python3
"""Fast static validation that does not require Unreal Engine."""

from __future__ import annotations

import csv
import io
import json
import math
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Content" / "Data"
CSV_SCHEMA_VERSION = 1
REGISTERED_BEHAVIOR_IDS = {
    "None",
    "RuntimeSmoke.LogValue",
    "Character.StaticStat",
    "Character.PersistentGrowth",
    "Character.EveryNth",
    "Character.MissingHealthSteps",
    "Card.StatModifier",
    "Card.InstantRecovery",
    "Card.GrantTier",
    "Card.RandomStatTrade",
    "Card.RandomRateTrade",
    "Card.ResetRunes",
    "Card.FreeShopVisit",
    "Card.UnlimitedShopRefresh",
    "Card.CollectCores",
    "Card.NumericChallenge",
    "Card.OverkillHeal",
    "Card.SelfRace",
    "Card.NegativeStatusHeal",
    "Card.TargetKillCurse",
    "Card.RecordReaction",
    "Card.CurseBank",
    "Card.ConnectionLine",
    "Card.ProximityDamage",
    "Card.WeaponMaster",
    "Card.EchoTrinityHead",
    "Card.EchoTrinityBody",
    "Card.EchoTrinityLegs",
    "Card.InfiniteStackingBurn",
    "Card.VaporizeWaterSplash",
    "Card.ConductDamageGrowth",
    "Card.OverhealCapacity",
    "Card.AlternatingSources",
    "Card.TrackNextKills",
    "Card.TrackNextReactions",
    "Card.EchoElementAura",
    "Card.EchoSlowAura",
    "Card.ElementAttachedCrit",
    "Card.ReactionHeal",
    "Card.DamageSubstitution",
    "Card.NextShardDrop",
    "Card.ShopContract",
    "Card.EncounterStun",
    "Card.DoubleEcho",
    "Card.TimeAnchor",
    "Card.SoloBody",
    "Card.TauntEcho",
    "Card.SoulResonance",
    "Card.EnemyImmunity",
    "Card.CriticalElement",
    "Card.ElementCanCrit",
    "Card.DistanceDamage",
    "Card.DoubleCritRoll",
    "Card.BloodForging",
    "Card.EndKillRefresh",
    "Card.HarvestPenalty",
    "Card.ShardOutgoingDamage",
    "Card.ShardIncomingBarrier",
    "Card.ReactionDiversity",
    "Card.DoubleNonCoreSlots",
    "Status.ElementImmunity",
    "Status.Burn",
    "Status.Stun",
    "Status.Bleeding",
    "Status.Cursed",
    "Reaction.Burn",
    "Reaction.Vaporize",
    "Reaction.Growth",
    "Reaction.Conduct",
    "Reaction.Enhance",
    "Weapon.AttackStep",
    "Weapon.DashStrike",
    "Part.CoreDamageChannel",
    "Part.StatModifier",
    "Part.AttackPatternReplacement",
    "Part.OnKillHealPercent",
    "Part.ProjectileSplitOnHit",
    "Part.ProjectilePierceOnCritical",
    "Part.ApplyBleedOnCritical",
    "Part.DropShardOnKill",
    "Part.MoveSpeedOnKill",
    "Part.AttackMoveSpeedOnAttack",
    "Part.RangeOnGroupHit",
    "Part.HealOnHit",
    "Part.StunOnHit",
    "Part.BleedEveryTargetHits",
    "Part.OuterRingDamage",
    "Part.MoveSpeedPerHit",
    "Part.InvulnerableOnGroupHit",
    "Part.AttackSpeedPerHit",
    "Part.ScytheThrowRecall",
    "Part.DropShardEveryHits",
    "Part.MeteorOnGroupHit",
    "Part.ApplyBleedOnHitChance",
    "Part.AttackSpeedOnAttack",
    "Part.MoveSpeedOnAttack",
    "Enemy.Grunt",
    "Enemy.Shield",
    "Enemy.Bomber",
    "Enemy.Slime",
    "Enemy.Ranged",
    "Enemy.Elite",
    "Boss.TimeGuard",
    "Boss.MeleeSweep",
    "Boss.Projectile",
    "Boss.BlinkSlam",
    "Boss.PrayerBeam",
    "Boss.ElementCleanse",
    "Enemy.RangedBurst",
    "Enemy.EliteDash",
}
REGISTERED_EFFECT_KINDS = {
    "ScalarModifier",
    "StatModifier",
    "InstantRecovery",
    "CardBehavior",
    "ElementReaction",
    "ExtraCardChoice",
    "WeaponDamageChannel",
    "AttackPatternReplacement",
    "ParameterizedBehavior",
    "UniqueBehavior",
}
REGISTERED_FORMULA_IDS = {
    "None",
    "Element.ElementAttackDot",
    "Element.ElementAttackSquared",
    "Element.AttachInRadius",
    "Element.ChainElementAttack",
    "Element.EnhanceNextReaction",
    "Weapon.DamageCoefficient",
}
REGISTERED_ATTACK_PATTERN_IDS = {
    "None",
    "Pattern.LongSwordCombo",
    "Pattern.ScytheSweep",
    "Pattern.BowShot",
    "Pattern.GunShot",
    "Pattern.MoonStaffWave",
    "Pattern.ElementalProjectile",
}
VALUE_OPS = {"Add", "Multiply", "Override"}
CARD_TARGETS = {
    "HpMax",
    "HpPoint",
    "PhysicalAttack",
    "ElementalAttack",
    "Block",
    "AttackSpeed",
    "MovementSpeed",
    "EchoEfficiency",
    "CriticalRate",
    "CriticalEffect",
    "ReactionEfficiency",
    "Tier",
    "PhysicalOrElemental",
    "ReactionOrCritical",
    "Water",
    "Grass",
    "Damage",
    "TimeShards",
    "ShopDiscount",
    "HpMaxAndPoint",
    "Stun",
    "EchoCount",
    "AnchorRecording",
    "AllBaseStats",
    "EchoHealth",
    "PhysicalAndElementalAttack",
    "EnemyElementImmunity",
    "RandomElement",
    "ElementCanCrit",
    "CriticalRollCount",
    "MinimumGuaranteedTier",
    "FreeShopRefresh",
    "NonCoreSlotCapacity",
    "ShopPrice",
    "WeaponRunes",
    "WeaponRuneShop",
    "CoreCollection",
    "Status",
    "ShopCredit",
    "ConnectionLine",
    "WeaponHistory",
}
CARD_TRIGGERS = {
    "OnApply",
    "OnGrant",
    "OnEncounterStart",
    "OnEncounterTick",
    "OnEncounterEnd",
    "BeforeOutgoingHit",
    "BeforeIncomingHit",
    "OnHitResolved",
    "OnDamageResolved",
    "OnStatusApplied",
    "OnReaction",
    "OnPurchase",
    "OnInventoryChanged",
    "OnEchoKilled",
    "OnResolveEchoes",
    "OnCompileRules",
}
CARD_BEHAVIOR_IDS = {value for value in REGISTERED_BEHAVIOR_IDS if value.startswith("Card.")}
CARD_EFFECT_BEHAVIOR_PAIRS = {
    "StatModifier": "Card.StatModifier",
    "InstantRecovery": "Card.InstantRecovery",
}
ELEMENT_IDS = {"Flame", "Lightning", "Grass", "Water"}
ELEMENT_ROLES = {"Trigger", "Attachment"}
STATUS_BEHAVIOR_PAIRS = {
    "Z_Elemental_Immunity": "Status.ElementImmunity",
    "Z_Burn": "Status.Burn",
    "Z_Vertigo": "Status.Stun",
    "Z_Bleeding": "Status.Bleeding",
    "Z_Cursed": "Status.Cursed",
}
REACTION_BEHAVIOR_FORMULA_PAIRS = {
    "Reaction.Burn": "Element.ElementAttackDot",
    "Reaction.Vaporize": "Element.ElementAttackSquared",
    "Reaction.Growth": "Element.AttachInRadius",
    "Reaction.Conduct": "Element.ChainElementAttack",
    "Reaction.Enhance": "Element.EnhanceNextReaction",
}
WEAPON_TYPE_IDS = {"LongSword", "Scythe", "Bow", "Gun"}
WEAPON_TYPE_ALLOWED_VALUES = "LongSword|Scythe|Bow|Gun"
INPUT_SLOTS = {"None", "1", "2", "3", "4", "5", "6"}
WEAPON_EFFECT_TARGETS = {
    "DamageChannel",
    "AttackSpeed",
    "AttackIntervalSeconds",
    "DamageCoefficient",
    "AttackPattern",
    "OnKill",
    # Weapon-parameter StatModifier targets. Each one is landed by
    # ReEchoWeaponRuntime::ApplyPartEffect as a RuleFlag and read back by
    # BuildEffectiveWeaponDefinition, so the runtime honours them for real.
    # Do not extend this set before the matching C++ branch exists.
    "AttackRange",
    "AttackArc",
    "ProjectileCount",
    "ConcentrationDegrees",
    "ExplosionRadius",
    "RuntimeBehavior",
}
# EffectKind -> set of BehaviorIds the runtime accepts for that kind. A kind may map to
# several behaviours (UniqueBehavior in particular), so membership is checked, not equality.
PART_EFFECT_BEHAVIOR_PAIRS = {
    "WeaponDamageChannel": {"Part.CoreDamageChannel"},
    "StatModifier": {"Part.StatModifier"},
    "AttackPatternReplacement": {"Part.AttackPatternReplacement"},
    "UniqueBehavior": {value for value in REGISTERED_BEHAVIOR_IDS if value.startswith("Part.")}
    - {"Part.CoreDamageChannel", "Part.StatModifier", "Part.AttackPatternReplacement"},
}


class ValidationError(Exception):
    pass


@dataclass(frozen=True)
class CsvColumnSpec:
    kind: str
    required: bool = True
    min_value: float | None = None
    max_value: float | None = None
    reference_table: str | None = None


CSV_TABLES: dict[str, dict[str, CsvColumnSpec]] = {
    "RuntimeSmoke": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayNameKey": CsvColumnSpec("TextKey"),
        "Enabled": CsvColumnSpec("Bool"),
        "TestScalar": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "TestPercent": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "DistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=1000000.0),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "ModifierOp": CsvColumnSpec("ValueOp"),
    },
    "RuntimeSmokeEffects": {
        "Id": CsvColumnSpec("StableId"),
        "RuntimeRowId": CsvColumnSpec("ForeignKey", reference_table="RuntimeSmoke"),
        "ParamName": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
    },
    "Characters": {
        "Id": CsvColumnSpec("StableId"),
        "SourceWorkbookId": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "RoleId": CsvColumnSpec("StableId"),
        "PromotionPriority": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DefaultWeaponId": CsvColumnSpec("StableId"),
        "AppearanceId": CsvColumnSpec("StableId"),
        "HpMax": CsvColumnSpec("Float", min_value=1.0, max_value=100000.0),
        "PhysicalAttack": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ElementalAttack": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "AttackSpeed": CsvColumnSpec("Float", min_value=0.1, max_value=100.0),
        "MovementSpeed": CsvColumnSpec("Float", min_value=0.1, max_value=100.0),
        "CriticalRate": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "CriticalEffect": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "EchoEfficiency": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "ReactionEfficiency": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
    },
    "CharacterAliases": {
        "Id": CsvColumnSpec("StableId"),
        "CanonicalCharacterId": CsvColumnSpec("ForeignKey", reference_table="Characters"),
        "Reason": CsvColumnSpec("Text"),
    },
    "CharacterAbilities": {
        "Id": CsvColumnSpec("StableId"),
        "CharacterId": CsvColumnSpec("ForeignKey", reference_table="Characters"),
        "Order": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Trigger": CsvColumnSpec("StableId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "Target": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "Interval": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Cards": {
        "Id": CsvColumnSpec("StableId"),
        "SourceWorkbookId": CsvColumnSpec("StableId"),
        "Tier": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "Tags": CsvColumnSpec("StableIdList"),
        "PromotionRoleId": CsvColumnSpec("StableId"),
        "OfferGroup": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "Offerable": CsvColumnSpec("Bool"),
        "StackPolicy": CsvColumnSpec("StableId"),
        "ConflictPolicy": CsvColumnSpec("StableId"),
        "ReviewStatus": CsvColumnSpec("StableId"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "CardEffects": {
        "Id": CsvColumnSpec("StableId"),
        "CardId": CsvColumnSpec("ForeignKey", reference_table="Cards"),
        "Order": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Trigger": CsvColumnSpec("StableId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "Target": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "ParamName": CsvColumnSpec("StableId"),
        "ParamValue": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
    },
    "Elements": {
        "Id": CsvColumnSpec("StableId"),
        "SourceWorkbookId": CsvColumnSpec("StableId"),
        "Role": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "DisplayNameKey": CsvColumnSpec("TextKey"),
        "ColorHex": CsvColumnSpec("Text"),
        "VisualKey": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Statuses": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "StackPolicy": CsvColumnSpec("StableId"),
        "RefreshPolicy": CsvColumnSpec("StableId"),
        "MutexGroup": CsvColumnSpec("StableId"),
        "Tags": CsvColumnSpec("StableIdList"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Reactions": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "TriggerElementId": CsvColumnSpec("ForeignKey", reference_table="Elements"),
        "AttachmentElementId": CsvColumnSpec("ForeignKey", reference_table="Elements"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "FormulaId": CsvColumnSpec("FormulaId"),
        "DamageMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "DamageIncrease": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "StatusId": CsvColumnSpec("ForeignKey", reference_table="Statuses"),
        "StatusDurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "EnhancementMultiplier": CsvColumnSpec("Float", min_value=1.0, max_value=100.0),
        "CanCrit": CsvColumnSpec("Bool"),
        "AffectedByEchoEfficiency": CsvColumnSpec("Bool"),
        "ClearsAttachment": CsvColumnSpec("Bool"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "WeaponTypes": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Description": CsvColumnSpec("Text"),
        "BaseAttackPatternId": CsvColumnSpec("AttackPatternId"),
        "SlotProfileId": CsvColumnSpec("StableId"),
        "BaseIntervalSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=60.0),
        "BaseRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "BaseArcDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "BaseProjectileCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "BaseConcentrationDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "BaseExplosionRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ChainWindowSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=60.0),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Weapons": {
        "Id": CsvColumnSpec("StableId"),
        "WeaponTypeId": CsvColumnSpec("ForeignKey", reference_table="WeaponTypes"),
        "DisplayName": CsvColumnSpec("Text"),
        "VisualKey": CsvColumnSpec("StableId"),
        "InputSlot": CsvColumnSpec("StableId"),
        "StartSelectable": CsvColumnSpec("Bool"),
        "LoadoutOrder": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "AttackPatternId": CsvColumnSpec("AttackPatternId"),
        "AttackIntervalSeconds": CsvColumnSpec("Float", min_value=0.01, max_value=60.0),
        "DamageCoefficient": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ArcDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ProjectileCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "ConcentrationDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ExplosionRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "DataRevision": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "AttackSteps": {
        "Id": CsvColumnSpec("StableId"),
        "AttackPatternId": CsvColumnSpec("AttackPatternId"),
        "StepIndex": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=60.0),
        "DamageCoefficient": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ArcDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ProjectileCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "ConcentrationDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "ExplosionRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MovementCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "Invulnerable": CsvColumnSpec("Bool"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "FormulaId": CsvColumnSpec("FormulaId"),
        "ConditionId": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "OuterRingStartFraction": CsvColumnSpec("Float", min_value=0.0, max_value=1.0),
        "OuterRingBonusMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
    },
    "SlotTypes": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "SlotProfiles": {
        "Id": CsvColumnSpec("StableId"),
        "WeaponTypeId": CsvColumnSpec("StableId"),
        "SlotTypeId": CsvColumnSpec("ForeignKey", reference_table="SlotTypes"),
        "SlotCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Required": CsvColumnSpec("Bool"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "DisabledReason": CsvColumnSpec("Text", required=False),
    },
    "Parts": {
        "Id": CsvColumnSpec("StableId"),
        "PartId": CsvColumnSpec("StableId"),
        "WeaponTypeId": CsvColumnSpec("StableId"),
        "SlotTypeId": CsvColumnSpec("ForeignKey", reference_table="SlotTypes"),
        "DisplayName": CsvColumnSpec("Text", required=False),
        "Description": CsvColumnSpec("Text", required=False),
        "Rarity": CsvColumnSpec("StableId"),
        "Tags": CsvColumnSpec("StableIdList"),
        "Enabled": CsvColumnSpec("Bool"),
        "ReviewStatus": CsvColumnSpec("StableId"),
        "ImplementationStatus": CsvColumnSpec("StableId"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "ShopEnabled": CsvColumnSpec("Bool"),
        "ShopPrice": CsvColumnSpec("Int", min_value=0.0, max_value=100000.0),
    },
    "PartEffects": {
        "Id": CsvColumnSpec("StableId"),
        "PartId": CsvColumnSpec("ForeignKey", reference_table="Parts"),
        "Order": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Trigger": CsvColumnSpec("StableId"),
        "EffectKind": CsvColumnSpec("EffectKind"),
        "Target": CsvColumnSpec("StableId"),
        "ValueOp": CsvColumnSpec("ValueOp"),
        "Value": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "FormulaId": CsvColumnSpec("FormulaId"),
        "AttackPatternId": CsvColumnSpec("AttackPatternId"),
        "ParamName": CsvColumnSpec("StableId"),
        "ParamValue": CsvColumnSpec("Float", min_value=-100000.0, max_value=100000.0),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "CooldownSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "StackPolicy": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "DisabledReason": CsvColumnSpec("Text", required=False),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
    },
    "Enemies": {
        "Id": CsvColumnSpec("StableId"),
        "Archetype": CsvColumnSpec("StableId"),
        "BehaviorProfileId": CsvColumnSpec("BehaviorId"),
        "PresentationId": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "MaxHealth": CsvColumnSpec("Float", min_value=1.0, max_value=1000000.0),
        "MoveSpeedMultiplier": CsvColumnSpec("Float", min_value=0.01, max_value=10.0),
        "CollisionRadiusCm": CsvColumnSpec("Float", min_value=0.1, max_value=100000.0),
        "CollisionHalfHeightCm": CsvColumnSpec("Float", min_value=0.1, max_value=100000.0),
        "ContactDamage": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "AttackIntervalSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "ContactRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MovementStopDistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "HitReactionDurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "KnockbackSpeedCmPerSecond": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "KnockbackDrag": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "TriggerRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "DamageRadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "FuseSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "Boss": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
        "Phase2Enabled": CsvColumnSpec("Bool"),
        "Phase2TriggerRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "Phase2RequiredAttackCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Phase2TransformSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "Phase2TriggerMode": CsvColumnSpec("StableId", required=False),
        "Phase2HealthThresholdRatio": CsvColumnSpec("Float", required=False, min_value=0.0, max_value=1.0),
        "HateRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
    },
    "EnemyAbilities": {
        "Id": CsvColumnSpec("StableId"),
        "OwnerEnemyId": CsvColumnSpec("ForeignKey", reference_table="Enemies"),
        "BehaviorId": CsvColumnSpec("BehaviorId"),
        "SequenceOrder": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Enabled": CsvColumnSpec("Bool"),
        "Damage": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "WindupSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "ActiveSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "RecoverySeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "CooldownSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "MinRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MaxRangeCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "RadiusCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "WidthCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "LengthCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "ProjectileSpeedCmPerSecond": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "TeleportOffsetCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "TargetingMode": CsvColumnSpec("StableId"),
        "LockTiming": CsvColumnSpec("StableId"),
        "CleanseIntervalSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "ImmunitySeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
        "ProjectileCount": CsvColumnSpec("Int", min_value=1.0, max_value=64.0),
        "SpreadAngleDegrees": CsvColumnSpec("Float", min_value=0.0, max_value=360.0),
        "bMovementDuringCast": CsvColumnSpec("Bool"),
    },
    "BossPhases": {
        "Id": CsvColumnSpec("StableId"),
        "BossEnemyId": CsvColumnSpec("ForeignKey", reference_table="Enemies"),
        "PhaseIndex": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "TriggerSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "EchoPolicy": CsvColumnSpec("StableId"),
        "PhysicalAttackMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "ElementalAttackMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "AttackSpeedMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "MovementSpeedMultiplier": CsvColumnSpec("Float", min_value=0.0, max_value=100.0),
        "RefillHealthPolicy": CsvColumnSpec("StableId"),
        "PhaseMaxHealth": CsvColumnSpec("Float", min_value=0.0, max_value=1000000.0),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "EnemyCombatStats": {
        "Id": CsvColumnSpec("StableId"),
        "EnemyId": CsvColumnSpec("ForeignKey", reference_table="Enemies"),
        "CombatIndex": CsvColumnSpec("Int", min_value=1.0, max_value=8.0),
        "MaxHealth": CsvColumnSpec("Float", min_value=1.0, max_value=1000000.0),
        "ContactDamage": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "AttackIntervalSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
    },
    "EnemyShardDrops": {
        "EncounterIndex": CsvColumnSpec("Int", min_value=1.0, max_value=8.0),
        "MeleeMin": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "MeleeMax": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "RangedMin": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "RangedMax": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "EliteMin": CsvColumnSpec("Int", required=False, min_value=0.0, max_value=1000000.0),
        "EliteMax": CsvColumnSpec("Int", required=False, min_value=0.0, max_value=1000000.0),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "Stages": {
        "Id": CsvColumnSpec("StableId"),
        "StageIndex": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "SceneId": CsvColumnSpec("StableId"),
        "FirstEncounterIndex": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "LastEncounterIndex": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "PreserveEnemiesBetweenEncounters": CsvColumnSpec("Bool"),
        "ClearEnemiesOnEnter": CsvColumnSpec("Bool"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "Encounters": {
        "Id": CsvColumnSpec("StableId"),
        "EncounterIndex": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "StageId": CsvColumnSpec("ForeignKey", reference_table="Stages"),
        "DurationSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "EndCondition": CsvColumnSpec("StableId"),
        "EchoAnchorRatio": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "PlayerAnchorRatio": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "MeleeTargetingPolicy": CsvColumnSpec("StableId"),
        "RangedBurstLimit": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "RangedBurstWindowSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "EliteSkillConcurrency": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "ActiveUnitLimit": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "BossCountsTowardUnitLimit": CsvColumnSpec("Bool"),
        "ReplayPolicy": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "EncounterWaves": {
        "Id": CsvColumnSpec("StableId"),
        "EncounterId": CsvColumnSpec("ForeignKey", reference_table="Encounters"),
        "WaveIndex": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "TriggerSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "MeleeCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "RangedCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "EliteCount": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "BossEnemyId": CsvColumnSpec("ForeignKey", required=False, reference_table="Enemies"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "SpawnProfiles": {
        "Id": CsvColumnSpec("StableId"),
        "EnemyRole": CsvColumnSpec("StableId"),
        "EnemyId": CsvColumnSpec("ForeignKey", reference_table="Enemies"),
        "MinAnchorDistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MaxAnchorDistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MinSpacingCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "WarningLeadSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "DistributionPolicy": CsvColumnSpec("StableId"),
        "SpacingPolicy": CsvColumnSpec("StableId"),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "SpawnPolicy": {
        "Id": CsvColumnSpec("StableId"),
        "AnchorLeadSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "MinPlayerDistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "MinEchoDistanceCm": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "BoundaryPolicy": CsvColumnSpec("StableId"),
        "CandidatePolicy": CsvColumnSpec("StableId"),
        "PlayerPredictionPolicy": CsvColumnSpec("StableId"),
        "MultiEchoPolicy": CsvColumnSpec("StableId"),
        "MaxCandidateAttempts": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "Enabled": CsvColumnSpec("Bool"),
        "SourceSheet": CsvColumnSpec("Text"),
        "SourceRow": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "Notes": CsvColumnSpec("Text", required=False),
    },
    "AudioEvents": {
        "EventId": CsvColumnSpec("StableId"),
        "VariantId": CsvColumnSpec("StableId", required=False),
        "AssetPath": CsvColumnSpec("Text", required=False),
        "Bus": CsvColumnSpec("StableId"),
        "EventType": CsvColumnSpec("StableId"),
        "Spatial3D": CsvColumnSpec("Bool"),
        "BaseVolume": CsvColumnSpec("PercentDecimal", min_value=0.0, max_value=1.0),
        "PitchMin": CsvColumnSpec("Float", min_value=0.01, max_value=4.0),
        "PitchMax": CsvColumnSpec("Float", min_value=0.01, max_value=4.0),
        "CooldownSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=3600.0),
        "MaxConcurrency": CsvColumnSpec("Int", min_value=0.0, max_value=256.0),
        "Priority": CsvColumnSpec("Int", min_value=0.0, max_value=100.0),
        "PausePolicy": CsvColumnSpec("StableId"),
        "AttenuationMin": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "AttenuationMax": CsvColumnSpec("Float", min_value=0.0, max_value=100000.0),
        "StartTimeSeconds": CsvColumnSpec("Float", min_value=0.0, max_value=86400.0),
    },
    "Attributes": {
        "Id": CsvColumnSpec("StableId"),
        "DisplayName": CsvColumnSpec("Text"),
        "Tier": CsvColumnSpec("Int", min_value=1.0, max_value=2.0),
        "IconName": CsvColumnSpec("Text"),
        "ValueKind": CsvColumnSpec("StableId"),
        "DisplayOrder": CsvColumnSpec("Int", min_value=1.0, max_value=100.0),
        "Explanation": CsvColumnSpec("Text", required=False),
    },
    "shop_price_ranges": {
        "PriceCategory": CsvColumnSpec("StableId"),
        "MinPrice": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "MaxPrice": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
    },
    "shop_drop_levels": {
        "EncounterIndex": CsvColumnSpec("Int", min_value=1.0, max_value=1000000.0),
        "FreeTier": CsvColumnSpec("Int", required=False, min_value=1.0, max_value=3.0),
        "ShopTiers": CsvColumnSpec("StableIdList", required=False),
    },
    "shop_refresh_rules": {
        "RuleId": CsvColumnSpec("StableId"),
        "CardSlotRefreshLimit": CsvColumnSpec("Int", min_value=0.0, max_value=100.0),
        "WeaponRuneRefreshLimit": CsvColumnSpec("Int", min_value=0.0, max_value=100.0),
        "CardSlotRefreshCost": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
        "WeaponRuneRefreshCost": CsvColumnSpec("Int", min_value=0.0, max_value=1000000.0),
    },
}


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


def fail(message: str) -> None:
    raise ValidationError(message)


def load_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"{rel(path)}: {exc}")


def read_utf8_no_bom(path: Path) -> str:
    try:
        raw = path.read_bytes()
    except OSError as exc:
        fail(f"{rel(path)}: {exc}")
    if raw.startswith(b"\xef\xbb\xbf"):
        fail(f"{rel(path)}: CSV must be UTF-8 without BOM")
    try:
        return raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        fail(f"{rel(path)}: invalid UTF-8: {exc}")


def load_csv(path: Path) -> list[dict[str, str]]:
    text = read_utf8_no_bom(path)
    try:
        reader = csv.DictReader(io.StringIO(text), strict=True)
        if not reader.fieldnames:
            fail(f"{rel(path)}: CSV file is empty")
        headers = [header.strip() for header in reader.fieldnames]
        if len(headers) != len(set(headers)) or any(not header for header in headers):
            fail(f"{rel(path)}: header is empty or duplicated")
        rows: list[dict[str, str]] = []
        for row in reader:
            if row.get(None):
                fail(f"{rel(path)}:{reader.line_num}: row has more cells than headers")
            normalized = {header: (row.get(header) or "").strip() for header in reader.fieldnames}
            if any(value.startswith(("=", "@")) for value in normalized.values()):
                fail(f"{rel(path)}:{reader.line_num}: CSV cells must not contain spreadsheet formulas")
            normalized["__line__"] = str(reader.line_num)
            rows.append(normalized)
        return rows
    except csv.Error as exc:
        fail(f"{rel(path)}: {exc}")


def stable_id(value: str) -> bool:
    allowed = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-.")
    return bool(value) and value == value.strip() and all(ch in allowed for ch in value)


def parse_float(path: Path, row: dict[str, str], column: str, spec: CsvColumnSpec) -> float:
    value = row[column]
    line = row["__line__"]
    try:
        parsed = float(value)
    except ValueError:
        fail(f"{rel(path)}:{line}:{column}: value must be a finite number")
    if not math.isfinite(parsed):
        fail(f"{rel(path)}:{line}:{column}: value must be a finite number")
    if spec.min_value is not None and parsed < spec.min_value:
        fail(f"{rel(path)}:{line}:{column}: value {parsed} is below {spec.min_value}")
    if spec.max_value is not None and parsed > spec.max_value:
        fail(f"{rel(path)}:{line}:{column}: value {parsed} exceeds {spec.max_value}")
    return parsed


def validate_csv_schema() -> None:
    schema_path = DATA / "csv_schema.csv"
    rows = load_csv(schema_path)
    required_columns = {
        "SchemaVersion",
        "TableId",
        "ColumnName",
        "Type",
        "Required",
        "Min",
        "Max",
        "ReferencesTable",
        "AllowedValues",
        "Description",
    }
    if set(rows[0].keys()) - {"__line__"} != required_columns:
        fail(f"{rel(schema_path)}: schema columns do not match the contract")

    declared = {(row["TableId"], row["ColumnName"]) for row in rows}
    for row in rows:
        line = row["__line__"]
        if row["SchemaVersion"] != str(CSV_SCHEMA_VERSION):
            fail(f"{rel(schema_path)}:{line}: unsupported schema version {row['SchemaVersion']}")
        if row["Required"] not in {"true", "false"}:
            fail(f"{rel(schema_path)}:{line}: Required must be true or false")
    weapon_type_id_rows = [
        row for row in rows if row["TableId"] == "WeaponTypes" and row["ColumnName"] == "Id"
    ]
    if len(weapon_type_id_rows) != 1 or weapon_type_id_rows[0]["AllowedValues"] != WEAPON_TYPE_ALLOWED_VALUES:
        fail(f"{rel(schema_path)}: WeaponTypes.Id allowed values must be {WEAPON_TYPE_ALLOWED_VALUES}")
    for table_id, specs in CSV_TABLES.items():
        for column in specs:
            if (table_id, column) not in declared:
                fail(f"{rel(schema_path)}: missing contract row for {table_id}.{column}")


def validate_manifest(data_dir: Path) -> dict[str, Path]:
    manifest_path = data_dir / "reecho_data_manifest.csv"
    rows = load_csv(manifest_path)
    expected_headers = {"SchemaVersion", "TableId", "FileName", "PrimaryKey", "__line__"}
    if set(rows[0].keys()) != expected_headers:
        fail(f"{rel(manifest_path)}: manifest columns do not match the contract")

    entries: dict[str, Path] = {}
    for row in rows:
        line = row["__line__"]
        if row["SchemaVersion"] != str(CSV_SCHEMA_VERSION):
            fail(f"{rel(manifest_path)}:{line}: unsupported SchemaVersion {row['SchemaVersion']}")
        table_id = row["TableId"]
        if table_id not in CSV_TABLES:
            fail(f"{rel(manifest_path)}:{line}: unknown TableId {table_id!r}")
        if table_id in entries:
            fail(f"{rel(manifest_path)}:{line}: duplicate TableId {table_id!r}")
        expected_primary_key = next(iter(CSV_TABLES[table_id]))
        if row["PrimaryKey"] != expected_primary_key:
            fail(f"{rel(manifest_path)}:{line}: PrimaryKey must be {expected_primary_key}")
        file_name = row["FileName"]
        if "/" in file_name or "\\" in file_name:
            fail(f"{rel(manifest_path)}:{line}: FileName must stay inside its data directory")
        entries[table_id] = data_dir / file_name

    missing_tables = sorted(set(CSV_TABLES) - set(entries))
    if missing_tables:
        fail(f"{rel(manifest_path)}: missing tables: {', '.join(missing_tables)}")
    return entries


def validate_table(path: Path, table_id: str, references: dict[str, set[str]]) -> set[str]:
    specs = CSV_TABLES[table_id]
    rows = load_csv(path)
    headers = set(rows[0].keys()) - {"__line__"} if rows else set(specs)
    if headers != set(specs):
        fail(f"{rel(path)}: expected columns {sorted(specs)}, got {sorted(headers)}")

    ids: set[str] = set()
    for row in rows:
        line = row["__line__"]
        for column, spec in specs.items():
            value = row[column]
            if spec.required and not value:
                fail(f"{rel(path)}:{line}:{column}: required value is empty")
            if not value:
                continue
            if spec.kind in {"StableId", "BehaviorId", "EffectKind", "FormulaId", "ForeignKey"} and not stable_id(value):
                fail(f"{rel(path)}:{line}:{column}: invalid stable id")
            if spec.kind == "StableIdList":
                for part in value.split("|"):
                    if not stable_id(part):
                        fail(f"{rel(path)}:{line}:{column}: invalid stable id list entry")
            if spec.kind == "Bool" and value not in {"true", "false"}:
                fail(f"{rel(path)}:{line}:{column}: boolean must be lowercase true or false")
            if spec.kind in {"Float", "PercentDecimal", "Int"}:
                parse_float(path, row, column, spec)
            if spec.kind == "Int" and float(value) != round(float(value)):
                fail(f"{rel(path)}:{line}:{column}: value must be an integer")
            if spec.kind == "ValueOp" and value not in VALUE_OPS:
                fail(f"{rel(path)}:{line}:{column}: value operation must be Add, Multiply or Override")
            if spec.kind == "BehaviorId" and value not in REGISTERED_BEHAVIOR_IDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ behavior id {value!r}")
            if spec.kind == "EffectKind" and value not in REGISTERED_EFFECT_KINDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ effect kind {value!r}")
            if spec.kind == "FormulaId" and value not in REGISTERED_FORMULA_IDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ formula id {value!r}")
            if spec.kind == "AttackPatternId" and value not in REGISTERED_ATTACK_PATTERN_IDS:
                fail(f"{rel(path)}:{line}:{column}: unknown registered C++ attack pattern id {value!r}")
            if spec.kind == "ForeignKey" and value == "None" and table_id == "Reactions" and column == "StatusId":
                continue
            if spec.kind == "ForeignKey" and value not in references[spec.reference_table or ""]:
                fail(f"{rel(path)}:{line}:{column}: unknown reference {value!r}")

        primary_key = next(iter(specs))
        row_id = (
            f"{row['EventId']}\0{row['VariantId']}"
            if table_id == "AudioEvents"
            else row[primary_key]
        )
        if row_id in ids:
            duplicate_label = "EventId/VariantId" if table_id == "AudioEvents" else (
                "id" if primary_key == "Id" else primary_key
            )
            duplicate_value = (
                (row["EventId"], row["VariantId"])
                if table_id == "AudioEvents"
                else row_id
            )
            fail(f"{rel(path)}:{line}:{primary_key}: duplicate {duplicate_label} {duplicate_value!r}")
        ids.add(row_id)
    if not ids and table_id == "RuntimeSmoke":
        fail(f"{rel(path)}: RuntimeSmoke must contain at least one row")
    return ids


def validate_csv_package(data_dir: Path) -> None:
    entries = validate_manifest(data_dir)
    references: dict[str, set[str]] = {}
    references["RuntimeSmoke"] = validate_table(entries["RuntimeSmoke"], "RuntimeSmoke", references)
    references["RuntimeSmokeEffects"] = validate_table(entries["RuntimeSmokeEffects"], "RuntimeSmokeEffects", references)
    references["Characters"] = validate_table(entries["Characters"], "Characters", references)
    references["CharacterAliases"] = validate_table(entries["CharacterAliases"], "CharacterAliases", references)
    references["CharacterAbilities"] = validate_table(entries["CharacterAbilities"], "CharacterAbilities", references)
    references["Cards"] = validate_table(entries["Cards"], "Cards", references)
    references["CardEffects"] = validate_table(entries["CardEffects"], "CardEffects", references)
    references["Elements"] = validate_table(entries["Elements"], "Elements", references)
    references["Statuses"] = validate_table(entries["Statuses"], "Statuses", references)
    references["Reactions"] = validate_table(entries["Reactions"], "Reactions", references)
    references["WeaponTypes"] = validate_table(entries["WeaponTypes"], "WeaponTypes", references)
    references["Weapons"] = validate_table(entries["Weapons"], "Weapons", references)
    references["AttackSteps"] = validate_table(entries["AttackSteps"], "AttackSteps", references)
    references["SlotTypes"] = validate_table(entries["SlotTypes"], "SlotTypes", references)
    references["SlotProfiles"] = validate_table(entries["SlotProfiles"], "SlotProfiles", references)
    references["Parts"] = validate_table(entries["Parts"], "Parts", references)
    references["PartEffects"] = validate_table(entries["PartEffects"], "PartEffects", references)
    references["Enemies"] = validate_table(entries["Enemies"], "Enemies", references)
    references["EnemyAbilities"] = validate_table(entries["EnemyAbilities"], "EnemyAbilities", references)
    references["BossPhases"] = validate_table(entries["BossPhases"], "BossPhases", references)
    references["EnemyCombatStats"] = validate_table(entries["EnemyCombatStats"], "EnemyCombatStats", references)
    references["EnemyShardDrops"] = validate_table(entries["EnemyShardDrops"], "EnemyShardDrops", references)
    references["Stages"] = validate_table(entries["Stages"], "Stages", references)
    references["Encounters"] = validate_table(entries["Encounters"], "Encounters", references)
    references["EncounterWaves"] = validate_table(entries["EncounterWaves"], "EncounterWaves", references)
    references["SpawnProfiles"] = validate_table(entries["SpawnProfiles"], "SpawnProfiles", references)
    references["SpawnPolicy"] = validate_table(entries["SpawnPolicy"], "SpawnPolicy", references)
    references["AudioEvents"] = validate_table(entries["AudioEvents"], "AudioEvents", references)
    references["Attributes"] = validate_table(entries["Attributes"], "Attributes", references)
    references["shop_price_ranges"] = validate_table(entries["shop_price_ranges"], "shop_price_ranges", references)
    references["shop_drop_levels"] = validate_table(entries["shop_drop_levels"], "shop_drop_levels", references)
    references["shop_refresh_rules"] = validate_table(
        entries["shop_refresh_rules"], "shop_refresh_rules", references
    )
    validate_audio_events_domain(entries)
    validate_character_build_domain(data_dir, entries)
    validate_element_reaction_domain(data_dir, entries)
    validate_weapon_domain(data_dir, entries)
    validate_enemy_domain(data_dir, entries)
    validate_enemy_shard_drop_domain(entries)
    validate_encounter_domain(data_dir, entries)


def assemble_fixture_package(fixture_dir: Path, temp_root: Path) -> Path:
    assembled = temp_root / fixture_dir.name
    assembled.mkdir(parents=True, exist_ok=True)
    for source in DATA.glob("*.csv"):
        shutil.copy2(source, assembled / source.name)
    for override in fixture_dir.glob("*.csv"):
        shutil.copy2(override, assembled / override.name)
    return assembled


def validate_audio_events_domain(entries: dict[str, Path]) -> None:
    path = entries["AudioEvents"]
    rows = load_csv(path)
    required_ids = {
        "Music.Menu", "Music.Encounter", "Music.Boss", "Music.Shop", "Music.Death", "Music.Victory",
        "Ambience.Arena", "Ambience.Rain",
        "UI.Hover", "UI.Confirm", "UI.Cancel", "UI.Error", "UI.Purchase", "UI.CardSelect",
        "UI.CardReveal", "UI.Equip", "UI.Unequip",
        "Combat.Attack", "Combat.Hit", "Combat.Block", "Combat.Hurt", "Combat.Kill", "Combat.Death",
        "Combat.Reaction", "Item.Pickup", "Flow.Victory",
        "Enemy.Spawn", "Enemy.Attack", "Enemy.Death", "Boss.Spawn", "Boss.Attack", "Boss.Death",
        "Echo.Spawn", "Echo.Attack", "Echo.End", "CameraMove", "Revive",
    }
    actual_ids = {row["EventId"] for row in rows}
    missing_ids = sorted(required_ids - actual_ids)
    unknown_ids = sorted(actual_ids - required_ids)
    if missing_ids:
        fail(f"{rel(path)}: missing stable audio event ids: {', '.join(missing_ids)}")
    if unknown_ids:
        fail(f"{rel(path)}: unknown audio event ids: {', '.join(unknown_ids)}")
    allowed_variants = {
        "Music.Encounter": {"", "Stage.1", "Stage.2", "Stage.3"},
        "Combat.Attack": {"", "W_J_01", "W_J_04", "W_J_08", "W_J_09"},
        "Combat.Hit": {"", "Flame", "Lightning", "Grass", "Water"},
        "Combat.Reaction": {
            "Reaction.Burn", "Reaction.Vaporize", "Reaction.Growth", "Reaction.Conduct", "Reaction.Enhance",
        },
    }
    for row in rows:
        line = row["__line__"]
        variant_id = row["VariantId"]
        if variant_id and row["EventId"] not in allowed_variants:
            fail(f"{rel(path)}:{line}:VariantId: {row['EventId']} does not support variants")
        if row["EventId"] in allowed_variants and variant_id not in allowed_variants[row["EventId"]]:
            fail(f"{rel(path)}:{line}:VariantId: unsupported {row['EventId']} variant {variant_id!r}")
        asset_path = row["AssetPath"]
        if asset_path and not re.fullmatch(r"/Game/[A-Za-z0-9_./-]+\.[A-Za-z0-9_]+", asset_path):
            fail(f"{rel(path)}:{line}:AssetPath: malformed Unreal soft object path")
        if row["Bus"] not in {"Music", "Ambience", "CombatSfx", "UiSfx"}:
            fail(f"{rel(path)}:{line}:Bus: unsupported audio bus {row['Bus']!r}")
        if row["EventType"] not in {"OneShot", "Loop"}:
            fail(f"{rel(path)}:{line}:EventType: must be OneShot or Loop")
        if row["PausePolicy"] not in {"PauseWithGame", "ContinueOnPause"}:
            fail(f"{rel(path)}:{line}:PausePolicy: unsupported pause policy")
        if float(row["PitchMin"]) > float(row["PitchMax"]):
            fail(f"{rel(path)}:{line}:PitchMax: must be >= PitchMin")
        if float(row["AttenuationMin"]) > float(row["AttenuationMax"]):
            fail(f"{rel(path)}:{line}:AttenuationMax: must be >= AttenuationMin")
        if row["Spatial3D"] == "false" and (float(row["AttenuationMin"]) != 0 or float(row["AttenuationMax"]) != 0):
            fail(f"{rel(path)}:{line}: non-spatial events must use zero attenuation")


def validate_character_build_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    characters = load_csv(entries["Characters"])
    abilities = load_csv(entries["CharacterAbilities"])
    cards = load_csv(entries["Cards"])
    effects = load_csv(entries["CardEffects"])
    enabled_characters = {row["Id"] for row in characters if row["Enabled"] == "true"}
    expected_characters = {"J_SPADE", "J_DIAMOND", "J_CLOVER", "J_HEART"}
    if expected_characters - enabled_characters:
        fail(
            f"{rel(entries['Characters'])}: production playable character ids must include "
            f"{sorted(expected_characters)}"
        )
    if data_dir.resolve() == DATA.resolve() and enabled_characters != expected_characters:
        fail(
            f"{rel(entries['Characters'])}: production playable character ids must be exactly "
            f"{sorted(expected_characters)}"
        )
    for row in characters:
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Characters'])}:{row['__line__']}: disabled character requires DisabledReason")

    expected_ability_ids = {
        "HUNTER_MOVE", "HUNTER_CRIT_RATE", "HUNTER_CRIT_EFFECT", "POET_REACTION_GROWTH",
        "SAGE_BONUS_CHOICE", "BRAVE_PHYSICAL", "BRAVE_ELEMENTAL",
    }
    enabled_ability_ids = {row["Id"] for row in abilities if row["Enabled"] == "true"}
    if data_dir.resolve() == DATA.resolve() and enabled_ability_ids != expected_ability_ids:
        fail(f"{rel(entries['CharacterAbilities'])}: canonical character ability ids changed: {sorted(enabled_ability_ids)}")
    seen_ability_orders: set[tuple[str, str, str]] = set()
    for row in abilities:
        key = (row["CharacterId"], row["Trigger"], row["Order"])
        if key in seen_ability_orders:
            fail(f"{rel(entries['CharacterAbilities'])}:{row['__line__']}: duplicate CharacterId/Trigger/Order {key}")
        seen_ability_orders.add(key)
        behavior = row["BehaviorId"]
        target = row["Target"]
        interval = float(row["Interval"])
        value = float(row["Value"])
        valid = row["ValueOp"] == "Add"
        if behavior == "Character.StaticStat":
            valid = valid and row["Trigger"] == "OnBuildInitialized" and row["EffectKind"] == "StatModifier"
            valid = valid and target in {"MovementSpeed", "CriticalRate", "CriticalEffect"} and interval == 0
        elif behavior == "Character.PersistentGrowth":
            valid = valid and row["Trigger"] == "OnEncounterCompleted" and row["EffectKind"] == "StatModifier"
            valid = valid and target == "ReactionEfficiency" and interval == 0
        elif behavior == "Character.EveryNth":
            valid = valid and row["Trigger"] == "OnTraitChoiceApplied" and row["EffectKind"] == "ExtraCardChoice"
            valid = valid and target == "TraitCardChoice" and interval >= 1 and interval.is_integer()
            valid = valid and value > 0 and value.is_integer()
        elif behavior == "Character.MissingHealthSteps":
            valid = valid and row["Trigger"] == "OnHealthChanged" and row["EffectKind"] == "StatModifier"
            valid = valid and target in {"PhysicalAttack", "ElementalAttack"} and value >= 0 and 0 < interval <= 1
        else:
            valid = False
        if not valid:
            fail(
                f"{rel(entries['CharacterAbilities'])}:{row['__line__']}: unsupported character ability "
                "trigger/effect/target/value combination"
            )
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['CharacterAbilities'])}:{row['__line__']}: disabled ability requires DisabledReason")

    enabled_cards = {row["Id"] for row in cards if row["Enabled"] == "true"}
    if any(row["OfferGroup"] == "Forge" or row["Id"].startswith("FORGE_") for row in cards):
        fail(f"{rel(entries['Cards'])}: removed Forge offers reappeared")
    offerable_traits = [row["Id"] for row in cards if row["OfferGroup"] == "Trait" and row["Offerable"] == "true"]
    expected_traits = [
        "G_1_01", "G_1_02", "G_1_03", "G_1_04", "G_1_05", "G_1_06", "G_1_07", "G_1_08",
        "G_2_04", "G_2_05", "G_2_06", "G_2_07", "G_2_08", "G_2_09", "G_2_10", "G_2_12",
        "G_2_13", "G_2_14", "G_2_15", "G_2_16", "G_2_17", "G_3_01", "G_3_02", "G_3_03",
        "G_3_04", "G_3_05", "G_3_07", "G_3_09", "G_3_10", "G_3_11", "G_3_12", "G_3_13",
        "G_3_14", "G_3_16", "G_3_17", "G_3_20", "G_3_21", "G_3_22", "G_2_23", "G_3_23",
        "G_2_21", "G_2_22", "G_2_28", "G_3_26", "G_2_18", "G_2_20", "G_2_29", "G_3_27", "G_2_27",
        "G_2_35", "G_2_36",
        "G_3_24",
        "G_2_19", "G_2_30", "G_3_25", "G_3_28", "G_3_29",
        "G_2_24", "G_2_25", "G_2_26", "G_2_31", "G_2_32", "G_2_33", "G_2_34",
    ]
    if offerable_traits != expected_traits:
        fail(f"{rel(entries['Cards'])}: staged 64-card offerable trait pool changed: {offerable_traits}")
    tier_counts = {
        tier: sum(1 for row in cards if row["OfferGroup"] == "Trait" and row["Tier"] == tier and row["Enabled"] == "true")
        for tier in ("1", "2", "3")
    }
    if tier_counts != {"1": 8, "2": 32, "3": 24}:
        fail(f"{rel(entries['Cards'])}: canonical card tier counts changed: {tier_counts}")
    removed_ids = {"G_2_01", "G_2_02", "G_2_03", "G_2_11", "G_3_15", "G_3_18"}
    if removed_ids & {row["Id"] for row in cards}:
        fail(f"{rel(entries['Cards'])}: removed legacy cards reappeared")
    silent_scale = next((row for row in cards if row["Id"] == "G_2_17"), None)
    if not silent_scale or silent_scale["DisplayName"] != "静默刻度":
        fail(f"{rel(entries['Cards'])}: G_2_17 must be 静默刻度")
    for row in cards:
        if row["Enabled"] == "true" and row["ReviewStatus"] != "Approved":
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: enabled card must be Approved")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: disabled card requires DisabledReason")
        if row["Offerable"] == "true" and row["Enabled"] != "true":
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: offerable card must be enabled")
        offer_tags = [tag for tag in row["Tags"].split("|") if tag.startswith("OfferEncounter")]
        if any(not re.fullmatch(r"OfferEncounter[1-8]", tag) for tag in offer_tags):
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: invalid encounter offer tag")
        if len(offer_tags) != len(set(offer_tags)):
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: duplicate encounter offer tag")
        if row["ConflictPolicy"] not in {
            "None", "EchoKeystone", "EchoElementAura", "EchoRecordingMode", "ElementCriticalMode"
        }:
            fail(f"{rel(entries['Cards'])}:{row['__line__']}: unknown conflict policy")

    effects_by_card: dict[str, list[dict[str, str]]] = {}
    seen_orders: set[tuple[str, str]] = set()
    for row in effects:
        if row["Trigger"] not in CARD_TRIGGERS:
            fail(f"{rel(entries['CardEffects'])}:{row['__line__']}: unsupported card effect trigger {row['Trigger']!r}")
        if row["Target"] not in CARD_TARGETS:
            fail(f"{rel(entries['CardEffects'])}:{row['__line__']}: unknown card effect target {row['Target']!r}")
        expected_behavior = CARD_EFFECT_BEHAVIOR_PAIRS.get(row["EffectKind"])
        valid_pair = row["BehaviorId"] == expected_behavior if expected_behavior else False
        if row["EffectKind"] == "CardBehavior":
            valid_pair = row["BehaviorId"] in CARD_BEHAVIOR_IDS - set(CARD_EFFECT_BEHAVIOR_PAIRS.values())
        if not valid_pair:
            fail(
                f"{rel(entries['CardEffects'])}:{row['__line__']}: invalid EffectKind/BehaviorId pair "
                f"{row['EffectKind']!r}/{row['BehaviorId']!r}"
            )
        key = (row["CardId"], row["Order"])
        if key in seen_orders:
            fail(f"{rel(entries['CardEffects'])}:{row['__line__']}: duplicate CardId/Order {key}")
        seen_orders.add(key)
        effects_by_card.setdefault(row["CardId"], []).append(row)
    for card_id in enabled_cards:
        if card_id not in effects_by_card:
            fail(f"{rel(entries['CardEffects'])}: enabled card {card_id!r} has no effect rows")


def validate_element_reaction_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    elements = load_csv(entries["Elements"])
    statuses = load_csv(entries["Statuses"])
    reactions = load_csv(entries["Reactions"])

    enabled_elements = {row["Id"] for row in elements if row["Enabled"] == "true"}
    if enabled_elements != ELEMENT_IDS:
        fail(f"{rel(entries['Elements'])}: enabled element ids changed: {sorted(enabled_elements)}")
    for row in elements:
        if row["Id"] not in ELEMENT_IDS:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: unsupported element id {row['Id']!r}")
        if row["Role"] not in ELEMENT_ROLES:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: role must be Trigger or Attachment")
        if not row["ColorHex"].startswith("#") or len(row["ColorHex"]) != 7:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: ColorHex must be #RRGGBB")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Elements'])}:{row['__line__']}: disabled element requires DisabledReason")

    status_ids = {row["Id"] for row in statuses}
    if {"Z_Elemental_Immunity", "Z_Burn"} - status_ids:
        fail(f"{rel(entries['Statuses'])}: elemental immunity and burn statuses are required")
    for row in statuses:
        expected_behavior = STATUS_BEHAVIOR_PAIRS.get(row["Id"], "None")
        if row["BehaviorId"] != expected_behavior:
            fail(
                f"{rel(entries['Statuses'])}:{row['__line__']}: invalid StatusId/BehaviorId pair "
                f"{row['Id']!r}/{row['BehaviorId']!r}"
            )
        if row["Enabled"] == "true" and row["BehaviorId"] == "None":
            fail(f"{rel(entries['Statuses'])}:{row['__line__']}: enabled status needs behavior")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Statuses'])}:{row['__line__']}: disabled status requires DisabledReason")

    expected_pairs = {
        ("Flame", "Grass"),
        ("Flame", "Water"),
        ("Lightning", "Grass"),
        ("Lightning", "Water"),
        ("Grass", "Water"),
        ("Water", "Grass"),
    }
    seen_pairs: set[tuple[str, str]] = set()
    enabled_reactions: set[str] = set()
    for row in reactions:
        pair = (row["TriggerElementId"], row["AttachmentElementId"])
        if pair in seen_pairs:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: duplicate ordered pair {pair}")
        seen_pairs.add(pair)
        if row["BehaviorId"] not in REACTION_BEHAVIOR_FORMULA_PAIRS:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: unsupported reaction behavior {row['BehaviorId']!r}")
        expected_formula = REACTION_BEHAVIOR_FORMULA_PAIRS[row["BehaviorId"]]
        if row["FormulaId"] != expected_formula:
            fail(
                f"{rel(entries['Reactions'])}:{row['__line__']}: invalid ReactionBehaviorId/FormulaId pair "
                f"{row['BehaviorId']!r}/{row['FormulaId']!r}"
            )
        if row["StatusId"] != "None" and row["StatusId"] not in status_ids:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: unknown status reference {row['StatusId']!r}")
        if row["Enabled"] == "true":
            enabled_reactions.add(row["Id"])
        if row["CanCrit"] == "true":
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: CanCrit is not supported yet")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Reactions'])}:{row['__line__']}: disabled reaction requires DisabledReason")
    if seen_pairs != expected_pairs:
        fail(f"{rel(entries['Reactions'])}: ordered reaction pairs changed: {sorted(seen_pairs)}")
    if len(enabled_reactions) != 6:
        fail(f"{rel(entries['Reactions'])}: expected six enabled reactions, got {sorted(enabled_reactions)}")


def validate_weapon_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    weapon_types = load_csv(entries["WeaponTypes"])
    weapons = load_csv(entries["Weapons"])
    attack_steps = load_csv(entries["AttackSteps"])
    slot_types = load_csv(entries["SlotTypes"])
    slot_profiles = load_csv(entries["SlotProfiles"])
    parts = load_csv(entries["Parts"])
    part_effects = load_csv(entries["PartEffects"])

    enabled_type_ids = {row["Id"] for row in weapon_types if row["Enabled"] == "true"}
    if enabled_type_ids != WEAPON_TYPE_IDS:
        fail(f"{rel(entries['WeaponTypes'])}: enabled weapon type ids changed: {sorted(enabled_type_ids)}")
    for row in weapon_types:
        if row["Id"] not in WEAPON_TYPE_IDS:
            fail(f"{rel(entries['WeaponTypes'])}:{row['__line__']}: unsupported WeaponTypeId {row['Id']!r}")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['WeaponTypes'])}:{row['__line__']}: disabled weapon type requires DisabledReason")

    enabled_weapons = {row["Id"]: row for row in weapons if row["Enabled"] == "true"}
    required_weapons = {"W_J_01", "W_J_04", "W_J_08", "W_J_09"}
    if set(enabled_weapons) != required_weapons or len(weapons) != len(required_weapons):
        fail(f"{rel(entries['Weapons'])}: production roster must be exactly the four retained weapons")
    start_selectable = sorted(
        [row for row in weapons if row["Enabled"] == "true" and row["StartSelectable"] == "true"],
        key=lambda row: int(row["LoadoutOrder"]),
    )
    if [row["Id"] for row in start_selectable] != ["W_J_04", "W_J_01", "W_J_08", "W_J_09"]:
        fail(f"{rel(entries['Weapons'])}: start weapon order must be Scythe, LongSword, Bow, Gun")
    input_map = {row["InputSlot"]: row["Id"] for row in weapons if row["InputSlot"] != "None" and row["Enabled"] == "true"}
    if input_map != {"1": "W_J_04", "2": "W_J_01", "3": "W_J_08", "5": "W_J_09"}:
        fail(f"{rel(entries['Weapons'])}: required four-weapon input slot mapping changed: {input_map}")
    if enabled_weapons["W_J_04"]["WeaponTypeId"] != "Scythe" or enabled_weapons["W_J_04"]["AttackPatternId"] != "Pattern.ScytheSweep":
        fail(f"{rel(entries['Weapons'])}: W_J_04 must use the Scythe pattern")
    for row in weapons:
        if row["InputSlot"] not in INPUT_SLOTS:
            fail(f"{rel(entries['Weapons'])}:{row['__line__']}: unsupported InputSlot {row['InputSlot']!r}")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['Weapons'])}:{row['__line__']}: disabled weapon requires DisabledReason")

    characters = load_csv(entries["Characters"])
    expected_character_weapons = {
        "J_DIAMOND": "W_J_08",
        "J_CLOVER": "W_J_08",
        "J_HEART": "W_J_04",
        "J_SPADE": "W_J_01",
    }
    for row in characters:
        if row["Enabled"] == "true" and row["DefaultWeaponId"] not in enabled_weapons:
            fail(
                f"{rel(entries['Characters'])}:{row['__line__']}: DefaultWeaponId "
                f"{row['DefaultWeaponId']!r} does not reference an enabled weapon"
            )
        if row["Enabled"] == "true" and expected_character_weapons.get(row["Id"]) != row["DefaultWeaponId"]:
            fail(f"{rel(entries['Characters'])}:{row['__line__']}: unexpected four-weapon character mapping")

    seen_steps: set[tuple[str, str]] = set()
    patterns_with_steps: set[str] = set()
    for row in attack_steps:
        key = (row["AttackPatternId"], row["StepIndex"])
        if key in seen_steps:
            fail(f"{rel(entries['AttackSteps'])}:{row['__line__']}: duplicate AttackPatternId/StepIndex {key}")
        seen_steps.add(key)
        if row["Enabled"] == "true":
            patterns_with_steps.add(row["AttackPatternId"])
        elif not row["DisabledReason"]:
            fail(f"{rel(entries['AttackSteps'])}:{row['__line__']}: disabled attack step requires DisabledReason")
        outer_start = float(row["OuterRingStartFraction"])
        outer_bonus = float(row["OuterRingBonusMultiplier"])
        if row["Id"] == "AS_SCYTHE_1":
            if outer_start != 0.5 or outer_bonus != 0.4:
                fail(f"{rel(entries['AttackSteps'])}:{row['__line__']}: scythe outer ring must be 0.5/+0.4")
        elif outer_start != 0.0 or outer_bonus != 0.0:
            fail(f"{rel(entries['AttackSteps'])}:{row['__line__']}: only the scythe base step may define an outer ring")
    missing_patterns = {row["AttackPatternId"] for row in weapons if row["Enabled"] == "true"} - patterns_with_steps
    if missing_patterns:
        fail(f"{rel(entries['AttackSteps'])}: enabled weapons missing attack steps: {sorted(missing_patterns)}")

    slot_type_ids = {row["Id"] for row in slot_types}
    retired_slot_types = {"StaffBody", "StaffCrystal", "WhipBody"}
    if slot_type_ids & retired_slot_types:
        fail(f"{rel(entries['SlotTypes'])}: retired staff/whip slot types remain")
    for row in slot_profiles:
        if row["WeaponTypeId"] != "Any" and row["WeaponTypeId"] not in WEAPON_TYPE_IDS:
            fail(f"{rel(entries['SlotProfiles'])}:{row['__line__']}: unknown WeaponTypeId {row['WeaponTypeId']!r}")
        if row["SlotTypeId"] not in slot_type_ids:
            fail(f"{rel(entries['SlotProfiles'])}:{row['__line__']}: unknown SlotTypeId {row['SlotTypeId']!r}")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['SlotProfiles'])}:{row['__line__']}: disabled slot profile requires DisabledReason")

    # Plan111 promotes the visible row-56 连射移速枪机 into a new stable production part while
    # retaining the old hidden row-57 audit placeholder for migration history.
    if len(parts) != 49:
        fail(f"{rel(entries['Parts'])}: four-weapon slot audit must contain 49 production/audit rows")
    named_rows = [row for row in parts if row["DisplayName"]]
    # The named/unnamed split is no longer pinned to 10/60: weapon part families are being
    # implemented incrementally, so naming + enabling rows is expected progress. The real
    # invariant kept here is that an unnamed audit row must stay inert (disabled, no PartId),
    # which still blocks half-migrated rows from reaching the runtime.
    for row in parts:
        if row["DisplayName"]:
            continue
        if row["Enabled"] != "false" or row["PartId"] != "None":
            fail(
                f"{rel(entries['Parts'])}:{row['__line__']}: unnamed audit row must stay disabled "
                f"with PartId None"
            )
    if not named_rows:
        fail(f"{rel(entries['Parts'])}: at least one named part row is required")
    enabled_parts = {row["Id"]: row for row in parts if row["Enabled"] == "true"}
    enabled_cores = [row for row in enabled_parts.values() if row["SlotTypeId"] == "Core"]
    if len(enabled_cores) < 6:
        fail(f"{rel(entries['Parts'])}: at least six generic cores must be enabled")
    for row in parts:
        if row["WeaponTypeId"] != "Any" and row["WeaponTypeId"] not in WEAPON_TYPE_IDS:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: unknown WeaponTypeId {row['WeaponTypeId']!r}")
        shop_enabled = row["ShopEnabled"] == "true"
        try:
            shop_price = int(row["ShopPrice"])
        except ValueError:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: ShopPrice must be an integer")
        if shop_enabled and (row["Enabled"] != "true" or row["ImplementationStatus"] != "Implemented"):
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: only enabled implemented parts may be sold")
        if shop_enabled and shop_price <= 0:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: shop-enabled part requires positive ShopPrice")
        if not shop_enabled and shop_price != 0:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: non-shop part must use ShopPrice 0")
        if row["Enabled"] == "true":
            if row["PartId"] == "None" or not row["DisplayName"]:
                fail(f"{rel(entries['Parts'])}:{row['__line__']}: enabled part needs stable PartId and name")
            if row["ReviewStatus"] != "Approved" or row["ImplementationStatus"] != "Implemented":
                fail(f"{rel(entries['Parts'])}:{row['__line__']}: enabled part must be approved and implemented")
        elif not row["DisabledReason"]:
            fail(f"{rel(entries['Parts'])}:{row['__line__']}: disabled part requires DisabledReason")

    effects_by_part: dict[str, list[dict[str, str]]] = {}
    seen_effect_orders: set[tuple[str, str]] = set()
    has_pattern_replacement = False
    has_unique_behavior = False
    has_value_ops: set[str] = set()
    for row in part_effects:
        if row["Target"] not in WEAPON_EFFECT_TARGETS:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: unsupported weapon effect target {row['Target']!r}")
        allowed_behaviors = PART_EFFECT_BEHAVIOR_PAIRS.get(row["EffectKind"])
        if not allowed_behaviors or row["BehaviorId"] not in allowed_behaviors:
            fail(
                f"{rel(entries['PartEffects'])}:{row['__line__']}: invalid EffectKind/BehaviorId pair "
                f"{row['EffectKind']!r}/{row['BehaviorId']!r}"
            )
        if row["Enabled"] == "true" and row["PartId"] not in enabled_parts:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: enabled effect references disabled part")
        if row["Enabled"] == "false" and not row["DisabledReason"]:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: disabled part effect requires DisabledReason")
        key = (row["PartId"], row["Order"])
        if key in seen_effect_orders:
            fail(f"{rel(entries['PartEffects'])}:{row['__line__']}: duplicate PartId/Order {key}")
        seen_effect_orders.add(key)
        effects_by_part.setdefault(row["PartId"], []).append(row)
        if row["Enabled"] == "true":
            has_value_ops.add(row["ValueOp"])
            has_pattern_replacement |= row["EffectKind"] == "AttackPatternReplacement"
            has_unique_behavior |= row["EffectKind"] == "UniqueBehavior"
    for part_id in enabled_parts:
        if part_id not in effects_by_part:
            fail(f"{rel(entries['PartEffects'])}: enabled part {part_id!r} has no effect rows")
    # NOTE: These invariants were historically satisfied only by the Dagger family
    # (PE_DAGGER_THRUST_PATTERN / PE_DAGGER_HOLY_KILL_HEAL). Plan 61 removes Dagger,
    # and no other weapon currently ships enabled AttackPatternReplacement /
    # UniqueBehavior parts, so the hard requirement is relaxed rather than forced.
    if not has_pattern_replacement:
        print(f"[NOTE] {rel(entries['PartEffects'])}: no enabled AttackPatternReplacement effect "
              f"(was Dagger-only; acceptable after Dagger removal)")
    if not has_unique_behavior:
        print(f"[NOTE] {rel(entries['PartEffects'])}: no enabled UniqueBehavior effect "
              f"(was Dagger-only; acceptable after Dagger removal)")
    # Add/Multiply value ops were historically exercised only by the Dagger family
    # (PE_DAGGER_STRENGTH_SPEED=Multiply, PE_DAGGER_HOLY_KILL_HEAL=Add). Plan 61 removes
    # Dagger, so only Override remains among enabled effects; require Override coverage
    # and note the missing Add/Multiply rather than hard-failing.
    if "Override" not in has_value_ops:
        fail(f"{rel(entries['PartEffects'])}: enabled part effects must cover Override at minimum")
    if has_value_ops != {"Add", "Multiply", "Override"}:
        print(f"[NOTE] {rel(entries['PartEffects'])}: enabled part effects cover {sorted(has_value_ops)} "
              f"(Add/Multiply were Dagger-only; acceptable after Dagger removal)")


def validate_enemy_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    enemies = load_csv(entries["Enemies"])
    abilities = load_csv(entries["EnemyAbilities"])
    phases = load_csv(entries["BossPhases"])
    archetypes = {"Grunt", "Shield", "Bomber", "Boss", "Slime", "Ranged", "Elite"}
    profiles = {
        "Grunt": "Enemy.Grunt",
        "Shield": "Enemy.Shield",
        "Bomber": "Enemy.Bomber",
        "Boss": "Boss.TimeGuard",
        "Slime": "Enemy.Slime",
        "Ranged": "Enemy.Ranged",
        "Elite": "Enemy.Elite",
    }
    required_ids = {"M_Grunt", "M_Shield", "M_Bomber", "M_SHEEP", "M_SLIME", "M_RABBIT", "M_FOX"}
    enabled_enemies = {row["Id"]: row for row in enemies if row["Enabled"] == "true"}
    if set(enabled_enemies) != required_ids:
        fail(f"{rel(entries['Enemies'])}: enabled enemy ids changed: {sorted(enabled_enemies)}")
    boss_ids: set[str] = set()
    for row in enemies:
        line = row["__line__"]
        archetype = row["Archetype"]
        if archetype not in archetypes:
            fail(f"{rel(entries['Enemies'])}:{line}:Archetype: unsupported archetype {archetype!r}")
        if row["BehaviorProfileId"] != profiles[archetype]:
            fail(f"{rel(entries['Enemies'])}:{line}:BehaviorProfileId: does not match Archetype {archetype!r}")
        is_boss = row["Boss"] == "true"
        if is_boss != (archetype == "Boss"):
            fail(f"{rel(entries['Enemies'])}:{line}:Boss: must match Boss archetype")
        if is_boss:
            boss_ids.add(row["Id"])
        bomber_values = tuple(float(row[field]) for field in ("TriggerRadiusCm", "DamageRadiusCm", "FuseSeconds"))
        if archetype == "Bomber" and any(value <= 0.0 for value in bomber_values):
            fail(f"{rel(entries['Enemies'])}:{line}:TriggerRadiusCm: Bomber radii and fuse must be positive")
        if archetype != "Bomber" and any(value != 0.0 for value in bomber_values):
            fail(f"{rel(entries['Enemies'])}:{line}:TriggerRadiusCm: non-Bomber fields must use explicit zero")

    allowed_behaviors = {
        "Boss.MeleeSweep",
        "Boss.Projectile",
        "Boss.BlinkSlam",
        "Boss.PrayerBeam",
        "Boss.ElementCleanse",
        "Enemy.RangedBurst",
        "Enemy.EliteDash",
    }
    active_orders: set[tuple[str, int]] = set()
    active_by_boss: dict[str, int] = {boss_id: 0 for boss_id in boss_ids}
    for row in abilities:
        line = row["__line__"]
        owner = row["OwnerEnemyId"]
        behavior = row["BehaviorId"]
        owner_enemy = enabled_enemies.get(owner)
        if owner_enemy is None:
            fail(f"{rel(entries['EnemyAbilities'])}:{line}:OwnerEnemyId: abilities require an enabled enemy owner")
        if behavior not in allowed_behaviors:
            fail(f"{rel(entries['EnemyAbilities'])}:{line}:BehaviorId: unsupported enemy behavior {behavior!r}")
        if behavior.startswith("Boss.") and owner not in boss_ids:
            fail(f"{rel(entries['EnemyAbilities'])}:{line}:OwnerEnemyId: Boss behavior requires a Boss owner")
        if behavior == "Enemy.RangedBurst" and owner_enemy["Archetype"] != "Ranged":
            fail(f"{rel(entries['EnemyAbilities'])}:{line}:OwnerEnemyId: RangedBurst requires a Ranged owner")
        if behavior == "Enemy.EliteDash" and owner_enemy["Archetype"] != "Elite":
            fail(f"{rel(entries['EnemyAbilities'])}:{line}:OwnerEnemyId: EliteDash requires an Elite owner")
        if float(row["MinRangeCm"]) > float(row["MaxRangeCm"]):
            fail(f"{rel(entries['EnemyAbilities'])}:{line}:MinRangeCm: cannot exceed MaxRangeCm")
        enabled = row["Enabled"] == "true"
        if behavior == "Boss.ElementCleanse":
            if int(row["SequenceOrder"]) != 0:
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:SequenceOrder: passive cleanse must use zero")
            neutral_fields = (
                "Damage", "WindupSeconds", "ActiveSeconds", "RecoverySeconds", "CooldownSeconds",
                "MinRangeCm", "MaxRangeCm", "RadiusCm", "WidthCm", "LengthCm",
                "ProjectileSpeedCmPerSecond", "TeleportOffsetCm",
            )
            if any(float(row[field]) != 0.0 for field in neutral_fields):
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:Damage: cleanse damage and spatial fields must be zero")
            if float(row["CleanseIntervalSeconds"]) <= 0.0 or float(row["ImmunitySeconds"]) <= 0.0:
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:CleanseIntervalSeconds: cleanse interval and immunity must be positive")
        else:
            order = int(row["SequenceOrder"])
            if order <= 0:
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:SequenceOrder: active abilities require a positive order")
            key = (owner, order)
            if key in active_orders:
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:SequenceOrder: duplicate owner/order {key}")
            active_orders.add(key)
            if enabled and owner in active_by_boss:
                active_by_boss[owner] += 1
            if float(row["CleanseIntervalSeconds"]) != 0.0 or float(row["ImmunitySeconds"]) != 0.0:
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:CleanseIntervalSeconds: active abilities must use explicit zero")
            if behavior == "Boss.Projectile" and float(row["ProjectileSpeedCmPerSecond"]) <= 0.0:
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:ProjectileSpeedCmPerSecond: projectile speed must be positive")
            if behavior == "Boss.BlinkSlam" and (float(row["WindupSeconds"]) <= 0.0 or float(row["RadiusCm"]) <= 0.0):
                fail(f"{rel(entries['EnemyAbilities'])}:{line}:RadiusCm: blink slam requires a warning and radius")
    missing_active = sorted(boss_id for boss_id, count in active_by_boss.items() if count == 0)
    if missing_active:
        fail(f"{rel(entries['EnemyAbilities'])}: bosses have no enabled active ability: {missing_active}")

    phase_keys: set[tuple[str, int]] = set()
    for row in phases:
        line = row["__line__"]
        boss_id = row["BossEnemyId"]
        if boss_id not in boss_ids:
            fail(f"{rel(entries['BossPhases'])}:{line}:BossEnemyId: phases require an enabled boss owner")
        key = (boss_id, int(row["PhaseIndex"]))
        if key in phase_keys:
            fail(f"{rel(entries['BossPhases'])}:{line}:PhaseIndex: duplicate boss phase {key}")
        phase_keys.add(key)
        # EchoPolicy drives the legacy timed echo-buff (TimeGuard). A blood-depleted boss such as M_SHEEP does not use
        # it (None), so allow both the legacy DestroyEncounterEchoes and None.
        if row["EchoPolicy"] not in {"DestroyEncounterEchoes", "None"}:
            fail(f"{rel(entries['BossPhases'])}:{line}:EchoPolicy: unsupported policy {row['EchoPolicy']!r}")
        # First-release rule: normal phases must not refill health. The only exception is a blood-depleted second phase
        # (M_SHEEP) that explicitly refills to its PhaseMaxHealth when transforming, which requires PhaseMaxHealth > 0.
        if row["RefillHealthPolicy"] == "RefillToMaximum":
            try:
                phase_max = float(row.get("PhaseMaxHealth", "0"))
            except ValueError:
                fail(f"{rel(entries['BossPhases'])}:{line}:PhaseMaxHealth: RefillToMaximum requires a numeric value")
            if phase_max <= 0.0:
                fail(f"{rel(entries['BossPhases'])}:{line}:PhaseMaxHealth: RefillToMaximum requires PhaseMaxHealth > 0")
        elif row["RefillHealthPolicy"] != "None":
            fail(f"{rel(entries['BossPhases'])}:{line}:RefillHealthPolicy: unsupported policy {row['RefillHealthPolicy']!r}")


def validate_enemy_shard_drop_domain(entries: dict[str, Path]) -> None:
    rows = load_csv(entries["EnemyShardDrops"])
    indices = [int(row["EncounterIndex"]) for row in rows]
    if indices != list(range(1, 9)):
        fail(f"{rel(entries['EnemyShardDrops'])}: EncounterIndex rows must be exactly 1..8 in order")
    for row in rows:
        line = row["__line__"]
        for minimum_field, maximum_field in (
            ("MeleeMin", "MeleeMax"),
            ("RangedMin", "RangedMax"),
        ):
            if int(row[minimum_field]) > int(row[maximum_field]):
                fail(
                    f"{rel(entries['EnemyShardDrops'])}:{line}:{minimum_field}: "
                    f"minimum cannot exceed maximum"
                )
        elite_min = row["EliteMin"]
        elite_max = row["EliteMax"]
        if bool(elite_min) != bool(elite_max):
            fail(
                f"{rel(entries['EnemyShardDrops'])}:{line}:EliteMin: "
                "EliteMin and EliteMax must both be blank or both configured"
            )
        if elite_min and int(elite_min) > int(elite_max):
            fail(f"{rel(entries['EnemyShardDrops'])}:{line}:EliteMin: minimum cannot exceed maximum")


def validate_boss_encounter_waves(boss_waves: list[dict[str, str]], encounter_waves_path: Path) -> None:
    """Validate Encounter.8's deterministic three-wave, single-Boss contract."""
    expected = [(1, 0.0), (2, 10.0), (3, 20.0)]
    if len(boss_waves) != len(expected):
        fail(
            f"{rel(encounter_waves_path)}: encounter 8 requires exactly three waves "
            "with WaveIndex 1/2/3 at 0/10/20 seconds"
        )

    for row, (expected_index, expected_trigger) in zip(boss_waves, expected, strict=True):
        line = row["__line__"]
        actual_index = int(row["WaveIndex"])
        if actual_index != expected_index:
            fail(
                f"{rel(encounter_waves_path)}:{line}:WaveIndex: encounter 8 expected "
                f"{expected_index}, got {actual_index}"
            )
        actual_trigger = float(row["TriggerSeconds"])
        if actual_trigger != expected_trigger:
            fail(
                f"{rel(encounter_waves_path)}:{line}:TriggerSeconds: encounter 8 WaveIndex "
                f"{expected_index} expected {expected_trigger:g}, got {actual_trigger:g}"
            )
        boss_enemy_id = row["BossEnemyId"]
        if expected_index == 1:
            if boss_enemy_id != "M_SHEEP":
                fail(
                    f"{rel(encounter_waves_path)}:{line}:BossEnemyId: encounter 8 WaveIndex 1 "
                    "must preserve stable boss id M_SHEEP"
                )
        elif boss_enemy_id:
            fail(
                f"{rel(encounter_waves_path)}:{line}:BossEnemyId: encounter 8 WaveIndex "
                f"{expected_index} is reinforcement-only and must not spawn a boss"
            )


def validate_encounter_domain(data_dir: Path, entries: dict[str, Path]) -> None:
    stages = load_csv(entries["Stages"])
    encounters = load_csv(entries["Encounters"])
    waves = load_csv(entries["EncounterWaves"])
    profiles = load_csv(entries["SpawnProfiles"])
    policies = load_csv(entries["SpawnPolicy"])

    enabled_stages = [row for row in stages if row["Enabled"] == "true"]
    if [int(row["StageIndex"]) for row in enabled_stages] != [1, 2, 3, 4]:
        fail(f"{rel(entries['Stages'])}: enabled StageIndex values must be exactly 1..4 in file order")
    expected_stage_ranges = [(1, 2), (3, 5), (6, 7), (8, 8)]
    stage_ranges: dict[str, tuple[int, int]] = {}
    expected_scene_ids = ["SC01", "SC02", "SC03", "SC04"]
    for row, expected_range, expected_scene_id in zip(
        enabled_stages, expected_stage_ranges, expected_scene_ids, strict=True
    ):
        line = row["__line__"]
        actual_range = (int(row["FirstEncounterIndex"]), int(row["LastEncounterIndex"]))
        if actual_range != expected_range:
            fail(f"{rel(entries['Stages'])}:{line}:FirstEncounterIndex: expected stage range {expected_range}, got {actual_range}")
        if row["SceneId"] != expected_scene_id:
            fail(
                f"{rel(entries['Stages'])}:{line}:SceneId: expected {expected_scene_id} "
                f"for StageIndex {row['StageIndex']}"
            )
        if row["ClearEnemiesOnEnter"] != "true":
            fail(f"{rel(entries['Stages'])}:{line}:ClearEnemiesOnEnter: stage entry must clear the previous roster")
        should_preserve = int(row["StageIndex"]) < 4
        if (row["PreserveEnemiesBetweenEncounters"] == "true") != should_preserve:
            fail(f"{rel(entries['Stages'])}:{line}:PreserveEnemiesBetweenEncounters: only stages 1..3 preserve enemies")
        stage_ranges[row["Id"]] = actual_range

    enabled_encounters = [row for row in encounters if row["Enabled"] == "true"]
    if [int(row["EncounterIndex"]) for row in enabled_encounters] != list(range(1, 9)):
        fail(f"{rel(entries['Encounters'])}: enabled EncounterIndex values must be exactly 1..8 in file order")
    encounter_by_id = {row["Id"]: row for row in enabled_encounters}
    for row in enabled_encounters:
        line = row["__line__"]
        encounter_index = int(row["EncounterIndex"])
        stage_range = stage_ranges[row["StageId"]]
        if not stage_range[0] <= encounter_index <= stage_range[1]:
            fail(f"{rel(entries['Encounters'])}:{line}:StageId: encounter index is outside its stage range")
        ratio_sum = float(row["EchoAnchorRatio"]) + float(row["PlayerAnchorRatio"])
        if abs(ratio_sum - 1.0) > 1e-6:
            fail(f"{rel(entries['Encounters'])}:{line}:EchoAnchorRatio: anchor ratios must sum to 1")
        if row["MeleeTargetingPolicy"] != "AllLocked":
            fail(f"{rel(entries['Encounters'])}:{line}:MeleeTargetingPolicy: unsupported policy {row['MeleeTargetingPolicy']!r}")
        if float(row["RangedBurstWindowSeconds"]) <= 0.0:
            fail(f"{rel(entries['Encounters'])}:{line}:RangedBurstWindowSeconds: must be positive")
        if encounter_index < 8:
            if row["EndCondition"] != "Duration" or float(row["DurationSeconds"]) != 30.0:
                fail(f"{rel(entries['Encounters'])}:{line}:EndCondition: encounters 1..7 require Duration=30 seconds")
            if row["BossCountsTowardUnitLimit"] != "true":
                fail(f"{rel(entries['Encounters'])}:{line}:BossCountsTowardUnitLimit: normal encounters use the standard cap")
        else:
            if row["EndCondition"] != "BossOrPlayerDeath" or float(row["DurationSeconds"]) != 0.0:
                fail(f"{rel(entries['Encounters'])}:{line}:EndCondition: encounter 8 must use BossOrPlayerDeath with zero duration")
            if row["BossCountsTowardUnitLimit"] != "false":
                fail(f"{rel(entries['Encounters'])}:{line}:BossCountsTowardUnitLimit: boss is additional to the reinforcement cap")

    waves_by_encounter: dict[str, list[dict[str, str]]] = {}
    for row in waves:
        if row["Enabled"] == "true":
            waves_by_encounter.setdefault(row["EncounterId"], []).append(row)
    if set(waves_by_encounter) != set(encounter_by_id):
        fail(f"{rel(entries['EncounterWaves'])}: every enabled encounter must own at least one enabled wave")
    for encounter_index in range(1, 8):
        encounter_id = f"Encounter.{encounter_index}"
        encounter_waves = waves_by_encounter[encounter_id]
        indexes = [int(row["WaveIndex"]) for row in encounter_waves]
        triggers = [float(row["TriggerSeconds"]) for row in encounter_waves]
        if indexes != [1, 2, 3] or triggers != [0.0, 10.0, 20.0]:
            line = encounter_waves[0]["__line__"]
            fail(f"{rel(entries['EncounterWaves'])}:{line}:WaveIndex: encounters 1..7 require waves 1..3 at 0/10/20 seconds")
        if any(row["BossEnemyId"] for row in encounter_waves):
            line = encounter_waves[0]["__line__"]
            fail(f"{rel(entries['EncounterWaves'])}:{line}:BossEnemyId: normal encounters cannot spawn a boss")
        if encounter_index <= 2 and any(int(row["EliteCount"]) != 0 for row in encounter_waves):
            line = encounter_waves[0]["__line__"]
            fail(f"{rel(entries['EncounterWaves'])}:{line}:EliteCount: encounters 1 and 2 cannot spawn elites")

    validate_boss_encounter_waves(waves_by_encounter["Encounter.8"], entries["EncounterWaves"])

    enabled_profiles = {row["EnemyRole"]: row for row in profiles if row["Enabled"] == "true"}
    expected_roles = {"Melee", "Ranged", "Elite", "BossReinforcement", "Boss"}
    if set(enabled_profiles) != expected_roles:
        fail(f"{rel(entries['SpawnProfiles'])}: enabled roles must be {sorted(expected_roles)}")
    for role, row in enabled_profiles.items():
        line = row["__line__"]
        if float(row["MinAnchorDistanceCm"]) > float(row["MaxAnchorDistanceCm"]):
            fail(f"{rel(entries['SpawnProfiles'])}:{line}:MinAnchorDistanceCm: cannot exceed MaxAnchorDistanceCm")
        if row["DistributionPolicy"] != "NormalAroundAnchor":
            fail(f"{rel(entries['SpawnProfiles'])}:{line}:DistributionPolicy: unsupported policy")
        expected_spacing = "UseEnemyRole" if role == "BossReinforcement" else "Explicit"
        if row["SpacingPolicy"] != expected_spacing:
            fail(f"{rel(entries['SpawnProfiles'])}:{line}:SpacingPolicy: expected {expected_spacing} for {role}")
    if enabled_profiles["Boss"]["EnemyId"] != "M_SHEEP":
        row = enabled_profiles["Boss"]
        fail(f"{rel(entries['SpawnProfiles'])}:{row['__line__']}:EnemyId: Boss profile must spawn M_SHEEP")

    enabled_policies = [row for row in policies if row["Enabled"] == "true"]
    if len(enabled_policies) != 1 or enabled_policies[0]["Id"] != "SpawnPolicy.Default":
        fail(f"{rel(entries['SpawnPolicy'])}: exactly one enabled SpawnPolicy.Default row is required")
    policy = enabled_policies[0]
    allowed_values = {
        "BoundaryPolicy": "MirrorInward",
        "CandidatePolicy": "DeterministicNormal",
        "PlayerPredictionPolicy": "LinearVelocity",
        "MultiEchoPolicy": "LatestEncounterE1",
    }
    for column, expected in allowed_values.items():
        if policy[column] != expected:
            fail(f"{rel(entries['SpawnPolicy'])}:{policy['__line__']}:{column}: expected {expected}")


def expect_fixture_failure(name: str, token: str) -> None:
    fixture = DATA / "TestFixtures" / "CsvRuntime" / name
    with tempfile.TemporaryDirectory(prefix="reecho_csv_fixture_") as temp:
        assembled = assemble_fixture_package(fixture, Path(temp))
        try:
            validate_csv_package(assembled)
        except ValidationError as exc:
            if token not in str(exc):
                fail(f"{rel(fixture)}: expected {token!r}, got {exc}")
            return
        fail(f"{rel(fixture)}: expected fixture to fail with {token}")


def validate_no_legacy_data_json() -> None:
    retired = {
        "global_balance.json",
        "characters.json",
        "weapons.json",
        "cards.json",
        "encounters.json",
        "elements.json",
        "reactions.json",
        "statuses.json",
        "enemies.json",
    }
    present = sorted(name for name in retired if (DATA / name).exists())
    if present:
        fail(
            "retired Content/Data JSON must not return after XLSX/CSV migration: "
            + ", ".join(present)
        )


def validate_workflow() -> None:
    project = load_json(ROOT / "ReEcho.uproject")
    modules = {module.get("Name") for module in project.get("Modules", [])}
    if "ReEcho" not in modules:
        fail("ReEcho.uproject does not declare the ReEcho runtime module")

    required_shared = {
        "AI_ONBOARDING.md",
        "WORKFLOW.md",
        "PROGRAMMER_RULES.md",
        "DESIGNER_RULES.md",
        "ARTIST_RULES.md",
        "SECRETARY_RULES.md",
        "GIT_RULES.md",
        "PLANNER_RULES.md",
        "EXECUTOR_RULES.md",
        "LESSONS.md",
        "PROJECT_RULES.md",
        "ARCHITECTURE.md",
        "CODEBASE_MAP.md",
    }
    absent_shared = sorted(name for name in required_shared if not (ROOT / "shared" / name).is_file())
    if absent_shared:
        fail(f"workflow deployment incomplete: {', '.join(absent_shared)}")
    if (ROOT / "shared" / "PLANNER_EXCHANGE.md").exists():
        fail("PLANNER_EXCHANGE.md is retired; use Plans, source, tests and Git")
    agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
    planner_rules = (ROOT / "shared" / "PLANNER_RULES.md").read_text(encoding="utf-8")
    executor_rules = (ROOT / "shared" / "EXECUTOR_RULES.md").read_text(encoding="utf-8")
    project_rules = (ROOT / "shared" / "PROJECT_RULES.md").read_text(encoding="utf-8")
    programmer_rules = (ROOT / "shared" / "PROGRAMMER_RULES.md").read_text(encoding="utf-8")
    designer_rules = (ROOT / "shared" / "DESIGNER_RULES.md").read_text(encoding="utf-8")
    artist_rules = (ROOT / "shared" / "ARTIST_RULES.md").read_text(encoding="utf-8")
    secretary_rules = (ROOT / "shared" / "SECRETARY_RULES.md").read_text(encoding="utf-8")
    git_rules = (ROOT / "shared" / "GIT_RULES.md").read_text(encoding="utf-8")
    workflow_text = (ROOT / "shared" / "WORKFLOW.md").read_text(encoding="utf-8")
    onboarding_text = (ROOT / "shared" / "AI_ONBOARDING.md").read_text(encoding="utf-8")
    plan_template = (ROOT / "plans" / "TEMPLATE.md").read_text(encoding="utf-8")
    readme_text = (ROOT / "README.md").read_text(encoding="utf-8")
    docs_workflow_text = (ROOT / "docs" / "AI_WORKFLOW.md").read_text(encoding="utf-8")
    architecture_root = ROOT / "shared" / "CODEBASE_MAP"
    architecture_path = architecture_root / "ARCHITECTURE.md"
    codebase_index_path = architecture_root / "README.md"
    module_template_path = architecture_root / "MODULE_TEMPLATE.md"
    for required_path in (architecture_path, codebase_index_path, module_template_path):
        if not required_path.is_file():
            fail(f"architecture library incomplete: {rel(required_path)}")
    architecture_text = architecture_path.read_text(encoding="utf-8")
    codebase_index_text = codebase_index_path.read_text(encoding="utf-8")
    module_template_text = module_template_path.read_text(encoding="utf-8")

    if "only mandatory reading-order authority" not in agents:
        fail("AGENTS.md must remain the sole startup-order authority")
    role_gate_markers = (
        "## First-contact professional-role gate",
        "你在本项目中的角色是程序、策划还是美术？",
        "Do not infer the role",
        "shared/PROGRAMMER_RULES.md",
        "shared/DESIGNER_RULES.md",
        "shared/ARTIST_RULES.md",
        "是否采用规划者-执行者模式？",
        "是否采用一任务一 worktree（每个任务一个独立文件夹）？",
    )
    missing_role_gate_markers = [marker for marker in role_gate_markers if marker not in agents]
    if missing_role_gate_markers:
        fail(f"AGENTS.md lacks the first-contact professional-role gate: {', '.join(missing_role_gate_markers)}")
    remote_rule_authority_markers = (
        "## 远端规则权威与权限升级",
        "最新远端权威规则高于最初 prompt",
        "向**程序侧项目秘书**请求确认",
        "不得自行猜测权限",
        "不替代程序、策划、美术或用户作产品取舍",
    )
    missing_remote_rule_authority_markers = [
        marker for marker in remote_rule_authority_markers if marker not in project_rules
    ]
    if missing_remote_rule_authority_markers:
        fail(
            "PROJECT_RULES.md lacks remote-rule authority or permission escalation: "
            + ", ".join(missing_remote_rule_authority_markers)
        )
    if "remote-rule authority and permission-escalation gate in `shared/PROJECT_RULES.md`" not in agents:
        fail("AGENTS.md must route prompt/rule conflicts to PROJECT_RULES.md")
    risk_authority_markers = (
        "## 风险操作的人类确认与执行",
        "风险操作授权的唯一权威",
        "覆盖或丢弃未提交内容",
        "跳过规则或 Plan 要求的构建、测试、审查或发布门禁",
        "建议先问程序",
        "确认后应直接执行",
        "经人工确认跳过/未验证",
        "不得写成通过",
        "人工确认不能授权伪造证据",
    )
    missing_risk_authority_markers = [marker for marker in risk_authority_markers if marker not in project_rules]
    if missing_risk_authority_markers:
        fail(
            "PROJECT_RULES.md lacks human-confirmed risk execution markers: "
            + ", ".join(missing_risk_authority_markers)
        )
    if "risky-operation prohibition or exception is governed" not in agents:
        fail("AGENTS.md must route every risky-operation rule to PROJECT_RULES.md")
    role_rule_markers = {
        "PROGRAMMER_RULES.md": (
            "程序用户路线",
            "Planner 负责规划",
            "Executor 负责按已发布 Plan 实现",
            "## Planner-Executor 模式",
            "选择 Planner-Executor 模式的唯一权威",
            "## 本地工作区模式",
            "选择“一任务一 worktree”的唯一权威",
            "所有大程序任务都必须",
            "shared/GIT_RULES.md",
        ),
        "DESIGNER_RULES.md": (
            "策划用户路线",
            "ReEchoData.xlsx",
            "策划 AI 可在本地读取、创建、修改、编译和试验仓库内任何文件",
            "禁止策划路线直接推送、合并或发布 `origin/main`",
            "shared/GIT_RULES.md",
        ),
        "ARTIST_RULES.md": ("美术用户路线", "本地工作方式自由", "禁止手改 `.uasset`", "shared/GIT_RULES.md"),
    }
    role_rule_texts = {
        "PROGRAMMER_RULES.md": programmer_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
    }
    for name, markers in role_rule_markers.items():
        missing = [marker for marker in markers if marker not in role_rule_texts[name]]
        if missing:
            fail(f"{name} lacks professional-route boundaries: {', '.join(missing)}")
    worktree_choice_markers = {
        "AGENTS.md": (
            "After that answer is known",
            "是否采用一任务一 worktree（每个任务一个独立文件夹）？",
            "independent of Planner-Executor mode",
        ),
        "PROGRAMMER_RULES.md": (
            "与 Planner-Executor 分工彼此独立",
            "未得到答案前停止程序工作",
            "不得直接在主工作区实现",
            "选择“否”后",
            "默认持续当前对话",
        ),
        "PROJECT_RULES.md": (
            "必须先完成 `PROGRAMMER_RULES.md` 的本地工作区模式确认",
            "用户明确选择后构成当前对话的本地执行约束",
        ),
        "EXECUTOR_RULES.md": ("按 `PROGRAMMER_RULES.md` 已确认的本地工作区模式执行",),
        "PLANNER_RULES.md": ("按 `PROGRAMMER_RULES.md` 已确认的本地工作区模式组织实现",),
        "AI_ONBOARDING.md": ("**程序本地工作区模式**",),
        "WORKFLOW.md": ("程序路线分别确认 Planner-Executor 分工和本地工作区模式",),
    }
    worktree_choice_texts = {
        "AGENTS.md": agents,
        "PROGRAMMER_RULES.md": programmer_rules,
        "PROJECT_RULES.md": project_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "PLANNER_RULES.md": planner_rules,
        "AI_ONBOARDING.md": onboarding_text,
        "WORKFLOW.md": workflow_text,
    }
    for name, markers in worktree_choice_markers.items():
        missing = [marker for marker in markers if marker not in worktree_choice_texts[name]]
        if missing:
            fail(f"{name} lacks the independent worktree-choice contract: {', '.join(missing)}")
    risk_route_marker = "风险性禁止项和例外统一遵循 `shared/PROJECT_RULES.md`"
    risk_route_texts = {
        "PROGRAMMER_RULES.md": programmer_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "SECRETARY_RULES.md": secretary_rules,
        "GIT_RULES.md": git_rules,
    }
    missing_risk_routes = [name for name, text_value in risk_route_texts.items() if risk_route_marker not in text_value]
    if missing_risk_routes:
        fail("rule files must route risky-operation confirmation to PROJECT_RULES.md: " + ", ".join(missing_risk_routes))
    secretary_markers = (
        "项目秘书仓库职责的唯一权威",
        "不是第四种用户专业角色",
        "可维护全仓库规则",
        "## 秘书任务与 Plan",
        "秘书身份不豁免大任务 Plan",
        "项目不维护 Exchange",
        "物理/Git 冲突、逻辑冲突和耦合",
        "负责受理各角色 AI 的规则冲突、职责路由和操作权限咨询",
        "推送 `origin/main`",
        "遵循 `shared/GIT_RULES.md` 中集中定义的身份和边界规则",
    )
    missing_secretary_markers = [marker for marker in secretary_markers if marker not in secretary_rules]
    if missing_secretary_markers:
        fail(f"SECRETARY_RULES.md lacks secretary authority markers: {', '.join(missing_secretary_markers)}")
    if "shared/SECRETARY_RULES.md" not in agents or "Project Secretary duty is a repository coordination overlay" not in agents:
        fail("AGENTS.md must route explicit Project Secretary duty to SECRETARY_RULES.md")
    if "项目秘书边界位于 `SECRETARY_RULES.md`" not in project_rules:
        fail("PROJECT_RULES.md must point to SECRETARY_RULES.md without redefining secretary duty")
    git_rule_markers = (
        "提交身份和发布完整性规则的唯一权威",
        "推送到任何远端引用",
        "[PROGRAMMER]",
        "[DESIGNER]",
        "[ARTIST]",
        "[SECRETARY]",
        "## origin/main 单发布者锁",
        "`main-publish-lock` 是程序路线和项目秘书发布 `origin/main` 时唯一允许的远端协调分支",
        "## 编号 Plan 单独发布例外",
        "发布编号 Plan 不需要获取、检查或等待 `main-publish-lock`",
        "即使远端锁已存在",
        "Plan 发布者不得创建、更新或删除锁分支",
        "只允许使用普通非强制 push 发布 `origin/main`",
        "--force-with-lease=refs/heads/main-publish-lock:",
        "未获锁时只允许 fetch",
        "不得先更新本地 main 后绕行合并",
        "必须等实际获锁后才开始本轮 main 发布集成",
        "获锁后必须重新 fetch `origin/main`",
        "最终候选必须是抢锁时提交的后代",
        "使用普通非强制 push 发布 main",
        "网络超时或结果不明时先查询远端",
        "不得仅按超时自动破锁",
        "程序路线每次推送 `origin/main` 前",
        "Build-Editor.cmd -Configuration Development -FullRebuild",
        "ReEchoEditor.prebuilt.json",
        "仅含源码的程序候选",
        "经人工确认跳过/未验证",
        "建议先问程序",
    )
    missing_git_rule_markers = [marker for marker in git_rule_markers if marker not in git_rules]
    if missing_git_rule_markers:
        fail(f"GIT_RULES.md lacks commit/publication authority markers: {', '.join(missing_git_rule_markers)}")
    if "shared/GIT_RULES.md" not in agents or "Commit or publication work" not in agents:
        fail("AGENTS.md must route commit and publication work to GIT_RULES.md")
    if "普通任务直接遵循 `AGENTS.md`" not in onboarding_text:
        fail("AI_ONBOARDING.md must not redefine ordinary-task startup order")
    if "所有风险操作遵循 `shared/PROJECT_RULES.md` 的统一确认机制" not in onboarding_text:
        fail("AI_ONBOARDING.md must route risky operations to PROJECT_RULES.md")
    for name, text in {
        "WORKFLOW.md": workflow_text,
        "PROJECT_RULES.md": project_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
    }.items():
        if "<待定>" in text or "<RESOURCE_LOCK>" in text:
            fail(f"{name} still contains a generic workflow placeholder")
    required_template_fields = (
        "## 协调",
        "Plan 编写方（AI 侧）：",
        "实现编写方（AI 侧）：",
        "任务状态：",
        "人工验收：",
        "本地规划 / 实现基线：",
        "影响模式：",
        "Writes:",
        "Stable Reads:",
        "兼容承诺 / 下游操作：",
        "明确排除：",
    )
    missing_template_fields = [field for field in required_template_fields if field not in plan_template]
    if missing_template_fields:
        fail(f"Plan template lacks impact and handoff fields: {', '.join(missing_template_fields)}")
    if "git rev-parse --path-format=absolute --git-common-dir" not in executor_rules:
        fail("Executor lock guidance must use the Git common directory shared by worktrees")
    if "Executor 更新其指定 Plan 的执行记录" not in project_rules:
        fail("project rules must keep routine shared-state writes out of Executor branches")
    if "不是第二本规则书" not in workflow_text:
        fail("WORKFLOW.md must remain explanatory rather than a duplicate rulebook")
    integration_audit_markers = (
        "## 外部提交集成审计",
        "物理/Git 冲突",
        "逻辑冲突",
        "耦合",
        "Git 可以快进或干净合并不代表没有逻辑冲突",
        "真实逻辑冲突",
        "main-publish-lock",
    )
    missing_audit_markers = [marker for marker in integration_audit_markers if marker not in planner_rules]
    if missing_audit_markers:
        fail(f"Planner rules lack the external-commit integration audit gate: {', '.join(missing_audit_markers)}")
    plan_publication_markers = (
        "## 执行前编号并发布 Plan",
        "所有大任务都必须先发布正式 Plan",
        "分配 Plan 编号前",
        "识别最大 Plan 编号",
        "立即将编号 Plan 单独发布到 `origin/main`",
        "按 `GIT_RULES.md` 的“编号 Plan 单独发布例外”执行",
        "不获取、检查或等待 `main-publish-lock`",
        "才开始实质实现",
        "若推送被拒或其他已发布 Plan 占用编号",
        "不需要任何 Exchange",
    )
    missing_plan_publication_markers = [marker for marker in plan_publication_markers if marker not in planner_rules]
    if missing_plan_publication_markers:
        fail(f"Planner rules lack remote-first Plan numbering/publication: {', '.join(missing_plan_publication_markers)}")
    plan_language_markers = (
        "## Plan 语言",
        "新建 Plan 及对现有 Plan 的实质性内容更新必须使用中文正文",
        "不得仅为翻译而批量重写已关闭历史 Plan",
    )
    missing_plan_language_markers = [marker for marker in plan_language_markers if marker not in planner_rules]
    if missing_plan_language_markers:
        fail(f"Planner rules lack the Chinese Plan language contract: {', '.join(missing_plan_language_markers)}")
    if "# Plan XX - <专业> - <简短名称>" not in plan_template or "## 锁定目标" not in plan_template:
        fail("Plan template must provide the Chinese authoring surface")
    if "所有大任务都必须先有正式 Plan" not in project_rules:
        fail("PROJECT_RULES.md must require remote-first Plans for every large task")
    local_autonomy_markers = {
        "PROJECT_RULES.md": (
            "## 本地自治与远端边界",
            "每个成员、AI 和克隆的本地工作自治",
            "分支和 worktree 是隔离建议，不是共享权限门禁",
            "能合并就合并、能推送就推送、能删除就删除",
            "鼓励小批量、较频繁地发布 main",
        ),
        "PROGRAMMER_RULES.md": ("两种模式使用同一套 Plan 和发布门禁", "所有大程序任务都必须"),
        "EXECUTOR_RULES.md": ("按 `PROGRAMMER_RULES.md` 已确认的本地工作区模式执行", "Plan 中的 `Isolated | ReadOnly | SharedContract | Exclusive` 只说明集成风险"),
        "GIT_RULES.md": ("本地 WIP 提交的名称、粒度和临时身份", "人类账号 + AI 身份"),
        "WORKFLOW.md": ("本地工作自由，远端边界严格", "不存在 Exchange"),
    }
    local_autonomy_texts = {
        "PROJECT_RULES.md": project_rules,
        "PROGRAMMER_RULES.md": programmer_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "GIT_RULES.md": git_rules,
        "WORKFLOW.md": workflow_text,
    }
    for name, markers in local_autonomy_markers.items():
        missing = [marker for marker in markers if marker not in local_autonomy_texts[name]]
        if missing:
            fail(f"{name} lacks local-autonomy/remote-boundary markers: {', '.join(missing)}")
    if "编号 Plan 在实现开始前发布到 `main`" not in workflow_text:
        fail("WORKFLOW.md must explain remote-first numbered Plan publication")
    compact_prompt_markers = (
        "启动提示只是中立路由外壳",
        "不得赋予新身份",
        "不改变你现有的身份或职责",
        "不得重复 Plan 的锁定目标",
    )
    missing_compact_prompt_markers = [marker for marker in compact_prompt_markers if marker not in planner_rules]
    if missing_compact_prompt_markers:
        fail(f"Planner rules lack the neutral Plan-driven prompt contract: {', '.join(missing_compact_prompt_markers)}")
    remote_branch_boundary_markers = {
        "PROJECT_RULES.md": (
            "`origin/main` 是唯一权威发布分支",
            "程序路线与项目秘书除 `GIT_RULES.md` 定义的临时 `main-publish-lock` 外仍只推送 `origin/main`",
            "`designer/<task>`",
            "`artist/<task>`",
            "协作分支不是发布面",
        ),
        "DESIGNER_RULES.md": ("`designer/<task>`", "不得直接推送或发布 `main`"),
        "ARTIST_RULES.md": ("`artist/<task>`", "不得直接推送或发布 `main`"),
        "PLANNER_RULES.md": ("`origin/main` 是唯一权威发布分支", "`main-publish-lock`", "Plan 编号冲突"),
        "EXECUTOR_RULES.md": ("`origin/main` 是唯一远端分支", "不自行推送任务分支"),
        "SECRETARY_RULES.md": ("临时 `main-publish-lock`", "普通非强制 push 发布 `origin/main`"),
        "WORKFLOW.md": ("远端 `main` 是唯一权威发布分支", "`main-publish-lock`", "`designer/<task>`", "`artist/<task>`", "编号 Plan 在实现开始前发布到 `main`"),
    }
    remote_branch_boundary_texts = {
        "PROJECT_RULES.md": project_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "SECRETARY_RULES.md": secretary_rules,
        "WORKFLOW.md": workflow_text,
    }
    for name, markers in remote_branch_boundary_markers.items():
        missing = [marker for marker in markers if marker not in remote_branch_boundary_texts[name]]
        if missing:
            fail(f"{name} lacks role-aware remote branch boundary markers: {', '.join(missing)}")
    live_remote_side_ref_files = {
        "AGENTS.md": agents,
        "PROJECT_RULES.md": project_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "WORKFLOW.md": workflow_text,
        "plans/TEMPLATE.md": plan_template,
    }
    for name, text_value in live_remote_side_ref_files.items():
        if "coord/" in text_value or "Planning broadcast" in text_value or "push only the task branch" in text_value:
            fail(f"{name} still advertises a remote planning/task side-ref workflow")
    non_authoritative_tag_sources = {
        "AGENTS.md": agents,
        "PROJECT_RULES.md": project_rules,
        "PROGRAMMER_RULES.md": programmer_rules,
        "DESIGNER_RULES.md": designer_rules,
        "ARTIST_RULES.md": artist_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "WORKFLOW.md": workflow_text,
        "AI_ONBOARDING.md": onboarding_text,
        "README.md": readme_text,
        "docs/AI_WORKFLOW.md": docs_workflow_text,
    }
    commit_tags = ("[PROGRAMMER]", "[DESIGNER]", "[ARTIST]", "[SECRETARY]")
    duplicate_tag_sources = [
        name
        for name, text_value in non_authoritative_tag_sources.items()
        if any(tag in text_value for tag in commit_tags)
    ]
    if duplicate_tag_sources:
        fail(f"AI commit-tag rules must live only in GIT_RULES.md, duplicated in: {', '.join(duplicate_tag_sources)}")
    if "Design/Data/ReEchoData.xlsx" not in readme_text or "generated into validated CSV" not in readme_text:
        fail("README.md must describe the current XLSX-to-CSV authority")
    if "Project Secretary route" not in readme_text or "SECRETARY_RULES.md" not in readme_text:
        fail("README.md must index the Project Secretary route")
    if "not a workflow authority" not in docs_workflow_text or "`shared/` is the collaboration control plane" not in docs_workflow_text:
        fail("docs/AI_WORKFLOW.md must remain a human pointer to the shared authority")
    if "Project Secretary coordination duty" not in docs_workflow_text or "SECRETARY_RULES.md" not in docs_workflow_text:
        fail("docs/AI_WORKFLOW.md must point to the Project Secretary authority")
    architecture_markers = (
        "## 当前 Runtime Module",
        "## 跨模块状态流",
        "## 跨模块不变量",
        "## 当前候选状态",
        "Design/Data/ReEchoData.xlsx",
        "旧玩法 JSON 已从 `Content/Data` 删除",
    )
    missing_architecture_markers = [marker for marker in architecture_markers if marker not in architecture_text]
    if missing_architecture_markers:
        fail(f"shared/CODEBASE_MAP/ARCHITECTURE.md is stale: {', '.join(missing_architecture_markers)}")
    index_markers = (
        "## Runtime Module 索引",
        "## 模块与内部领域索引",
        "## 文档职责",
        "## 同步规则",
        "MODULE_TEMPLATE.md",
    )
    missing_index_markers = [marker for marker in index_markers if marker not in codebase_index_text]
    if missing_index_markers:
        fail(f"shared/CODEBASE_MAP/README.md is stale: {', '.join(missing_index_markers)}")
    required_module_sections = (
        "## 模块状态",
        "## 存在原因",
        "## 职责与排除项",
        "## 权威状态",
        "## 输入、输出与公共契约",
        "## 依赖方向",
        "## 运行时流程",
        "## 代码位置与阅读路线",
        "## 扩展",
        "## 验证",
        "## 不变量与常见错误",
    )
    missing_template_sections = [section for section in required_module_sections if section not in module_template_text]
    if missing_template_sections:
        fail(f"shared/CODEBASE_MAP/MODULE_TEMPLATE.md lacks module sections: {', '.join(missing_template_sections)}")
    runtime_modules = sorted(
        module.get("Name")
        for module in project.get("Modules", [])
        if module.get("Type") == "Runtime" and module.get("Name")
    )
    module_document_texts = {}
    for module_name in runtime_modules:
        architecture_id = f"MOD-{module_name}"
        if architecture_id not in architecture_text:
            fail(f"shared/CODEBASE_MAP/ARCHITECTURE.md lacks runtime module id: {architecture_id}")
        if architecture_id not in codebase_index_text:
            fail(f"shared/CODEBASE_MAP/README.md lacks runtime module index: {architecture_id}")
        module_document_path = architecture_root / "modules" / f"{architecture_id}.md"
        if not module_document_path.is_file():
            fail(f"runtime module lacks detailed design document: {rel(module_document_path)}")
        module_document_text = module_document_path.read_text(encoding="utf-8")
        module_document_texts[architecture_id] = module_document_text
        if architecture_id not in module_document_text or f"Source/{module_name}/" not in module_document_text:
            fail(f"{rel(module_document_path)} lacks its id or code location")
        missing_sections = [section for section in required_module_sections if section not in module_document_text]
        if missing_sections:
            fail(f"{rel(module_document_path)} lacks detailed design sections: {', '.join(missing_sections)}")
    architecture_area_ids = (
        "AREA-Core",
        "AREA-Data",
        "AREA-AbilityCombat",
        "AREA-Weapons",
        "AREA-Enemies",
        "AREA-Encounter",
        "AREA-Run",
        "AREA-Recording",
        "AREA-Player",
        "AREA-Presentation",
        "AREA-UI",
        "AREA-Tests",
    )
    expected_module_ids = {f"MOD-{module_name}" for module_name in runtime_modules}
    architecture_module_ids = set(re.findall(r"`(MOD-[A-Za-z][A-Za-z0-9]*)`", architecture_text))
    if architecture_module_ids != expected_module_ids:
        fail(
            "runtime descriptor and architecture module ids differ: "
            f"descriptor={sorted(expected_module_ids)}, architecture={sorted(architecture_module_ids)}"
        )
    indexed_ids = set(re.findall(r"`((?:MOD|AREA)-[A-Za-z][A-Za-z0-9]*)`", codebase_index_text))
    documented_ids = set()
    for module_document_text in module_document_texts.values():
        documented_ids.update(re.findall(r"`((?:MOD|AREA)-[A-Za-z][A-Za-z0-9]*)`", module_document_text))
    for architecture_id in architecture_area_ids:
        if architecture_id not in codebase_index_text:
            fail(f"shared/CODEBASE_MAP/README.md lacks internal area index: {architecture_id}")
        if architecture_id not in documented_ids:
            fail(f"module design documents lack internal area design: {architecture_id}")
    if indexed_ids != documented_ids:
        fail(
            "architecture index and module documents use different stable ids: "
            f"index_only={sorted(indexed_ids - documented_ids)}, docs_only={sorted(documented_ids - indexed_ids)}"
        )
    architecture_maintenance_markers = (
        "## 架构影响与设计决策",
        "受影响架构标识",
        "对应模块文档",
        "相关文档同步范围",
        "`<文档>` 已更新",
        "`<文档>` 已审阅、无需修改",
        "### 架构文档审阅结果",
    )
    missing_architecture_maintenance_markers = [
        marker for marker in architecture_maintenance_markers if marker not in plan_template
    ]
    if missing_architecture_maintenance_markers:
        fail(
            "plans/TEMPLATE.md lacks architecture decision/closure records: "
            + ", ".join(missing_architecture_maintenance_markers)
        )
    if "未记录完整审阅结果不得关闭 Plan" not in project_rules:
        fail("PROJECT_RULES.md must gate Plan closure on architecture review")
    module_document_gate_markers = {
        "PROJECT_RULES.md": (
            "每个大程序任务在实现前必须把受影响的 `MOD-*` / `AREA-*`",
            "程序修改任一 Runtime Module 时",
            "直接修改模块及受契约影响模块",
        ),
        "PROGRAMMER_RULES.md": ("修改任何 Runtime Module 前", "modules/MOD-*.md"),
        "PLANNER_RULES.md": (
            "程序 Plan 在发布前必须完成“架构影响与设计决策”",
            "每份相关文档必须在 Plan 记录",
        ),
        "EXECUTOR_RULES.md": ("每个被修改 Runtime Module 的 `modules/MOD-*.md` 位于 Writes",),
        "CODEBASE_MAP/README.md": ("每个程序 Plan 在实施前必须列出受影响的 `MOD-*` / `AREA-*`",),
    }
    module_document_gate_texts = {
        "PROJECT_RULES.md": project_rules,
        "PROGRAMMER_RULES.md": programmer_rules,
        "PLANNER_RULES.md": planner_rules,
        "EXECUTOR_RULES.md": executor_rules,
        "CODEBASE_MAP/README.md": codebase_index_text,
    }
    for name, markers in module_document_gate_markers.items():
        missing = [marker for marker in markers if marker not in module_document_gate_texts[name]]
        if missing:
            fail(f"{name} lacks programmer module-document gates: {', '.join(missing)}")
    if "不得设为 `Closed`" not in planner_rules:
        fail("PLANNER_RULES.md must enforce architecture review during Plan closure")


def validate_build_dependencies() -> None:
    build_cs = (ROOT / "Source" / "ReEcho" / "ReEcho.Build.cs").read_text(encoding="utf-8")
    if '"ReEchoEnemies"' not in build_cs:
        fail("ReEcho host module must depend on ReEchoEnemies for EnemyHost composition")
    if '"ReEchoPresentation"' not in build_cs:
        fail("ReEcho host module must depend on ReEchoPresentation for 2D presentation composition")

    presentation_root = ROOT / "Source" / "ReEchoPresentation"
    presentation_build_path = presentation_root / "ReEchoPresentation.Build.cs"
    if not presentation_build_path.is_file():
        fail("missing ReEchoPresentation runtime module")
    presentation_build = presentation_build_path.read_text(encoding="utf-8")
    for required_module in ("Core", "CoreUObject", "Engine", "GameplayTags", "Paper2D"):
        if f'"{required_module}"' not in presentation_build:
            fail(f"ReEchoPresentation must depend on {required_module}")
    for forbidden_module in ("ReEcho", "ReEchoEnemies", "ReEchoCombat", "ReEchoWeapons"):
        if re.search(rf'"{re.escape(forbidden_module)}"', presentation_build):
            fail(f"ReEchoPresentation must not depend on {forbidden_module}")
    for source_path in presentation_root.rglob("*"):
        if source_path.suffix not in {".h", ".cpp"}:
            continue
        source_text = source_path.read_text(encoding="utf-8")
        for forbidden_token in ("AReEchoEnemyActor", "AReEchoPlayerPawn", "AReEchoGameMode"):
            if forbidden_token in source_text:
                fail(f"{rel(source_path)} has forbidden gameplay host dependency: {forbidden_token}")

    descriptor = load_json(ROOT / "ReEcho.uproject")
    default_game = (ROOT / "Config" / "DefaultGame.ini").read_text(encoding="utf-8")
    if '+DirectoriesToAlwaysCook=(Path="/Game/VFX")' not in default_game:
        fail("Config/DefaultGame.ini must always cook the runtime combat VFX directory")
    for runtime_ui_directory in (
        "/Game/ReEcho/Textures/UI/Cards",
        "/Game/ReEcho/Textures/UI/WeaponParts/Icons",
        "/Game/ReEcho/UI/CombatHud",
    ):
        if f'+DirectoriesToAlwaysCook=(Path="{runtime_ui_directory}")' not in default_game:
            fail(
                "Config/DefaultGame.ini must always cook dynamically loaded UI assets from "
                f"{runtime_ui_directory}"
            )
    presentation_modules = [
        module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoPresentation"
    ]
    if len(presentation_modules) != 1 or presentation_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoPresentation Runtime module")
    for file_name in (
        "reecho_data_manifest.csv",
        "csv_schema.csv",
        "runtime_smoke.csv",
        "runtime_smoke_effects.csv",
        "characters.csv",
        "character_aliases.csv",
        "cards.csv",
        "card_effects.csv",
        "elements.csv",
        "statuses.csv",
        "reactions.csv",
        "weapon_types.csv",
        "weapons.csv",
        "attack_steps.csv",
        "slot_types.csv",
        "slot_profiles.csv",
        "parts.csv",
        "part_effects.csv",
        "enemies.csv",
        "enemy_abilities.csv",
        "boss_phases.csv",
        "audio_events.csv",
    ):
        if f"Content/Data/{file_name}" not in build_cs:
            fail(f"ReEcho.Build.cs does not stage production CSV {file_name}")


def validate_combat_module_boundaries() -> None:
    combat_root = ROOT / "Source" / "ReEchoCombat"
    build_path = combat_root / "ReEchoCombat.Build.cs"
    if not build_path.is_file():
        fail("missing ReEchoCombat runtime module")
    build_text = build_path.read_text(encoding="utf-8")
    if '"ReEcho"' in build_text or '"ReEchoAudio"' in build_text:
        fail("ReEchoCombat must not depend on the main or audio runtime modules")

    forbidden_include_prefixes = (
        "Data/",
        "Graybox/",
        "Player/",
        "Presentation/",
        "Recording/",
        "Run/",
        "UI/",
        "Weapons/",
    )
    include_pattern = re.compile(r'^\s*#include\s+[<\"]([^>\"]+)[>\"]', re.MULTILINE)
    for path in combat_root.rglob("*"):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        for include in include_pattern.findall(text_value):
            if include.startswith(forbidden_include_prefixes) or include in {"ReEcho.h", "ReEchoAudio.h"}:
                fail(f"{rel(path)} has forbidden main/audio module include: {include}")

    descriptor = load_json(ROOT / "ReEcho.uproject")
    combat_modules = [module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoCombat"]
    if len(combat_modules) != 1 or combat_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoCombat Runtime module")

    weapons_root = ROOT / "Source" / "ReEchoWeapons"
    weapons_build_path = weapons_root / "ReEchoWeapons.Build.cs"
    if not weapons_build_path.is_file():
        fail("missing ReEchoWeapons runtime module")
    weapons_build_text = weapons_build_path.read_text(encoding="utf-8")
    if '"ReEchoCombat"' not in weapons_build_text:
        fail("ReEchoWeapons must depend on the public ReEchoCombat contracts")
    if '"ReEcho"' in weapons_build_text or '"ReEchoAudio"' in weapons_build_text:
        fail("ReEchoWeapons must not depend on the main or audio runtime modules")

    weapons_forbidden_prefixes = (
        "Data/",
        "Graybox/",
        "Player/",
        "Presentation/",
        "Recording/",
        "Run/",
        "UI/",
    )
    weapons_forbidden_logic_tokens = (
        "ApplyFinalDamage",
        "ReceiveGrayboxDamage",
        "ReceiveElementalDamage",
        "ReEchoGameplayEffects::ApplyDamage",
        "AReEchoEnemyActor",
        "AReEchoPlayerPawn",
    )
    for path in weapons_root.rglob("*"):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        for include in include_pattern.findall(text_value):
            if include.startswith(weapons_forbidden_prefixes) or include in {"ReEcho.h", "ReEchoAudio.h"}:
                fail(f"{rel(path)} has forbidden main/audio module include: {include}")
        for token in weapons_forbidden_logic_tokens:
            if token in text_value:
                fail(f"{rel(path)} bypasses Combat adjudication with forbidden token: {token}")

    weapons_modules = [module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoWeapons"]
    if len(weapons_modules) != 1 or weapons_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoWeapons Runtime module")

    enemies_root = ROOT / "Source" / "ReEchoEnemies"
    enemies_build_path = enemies_root / "ReEchoEnemies.Build.cs"
    if not enemies_build_path.is_file():
        fail("missing ReEchoEnemies runtime module")
    enemies_build_text = enemies_build_path.read_text(encoding="utf-8")
    if '"ReEchoCombat"' not in enemies_build_text:
        fail("ReEchoEnemies must depend on the public ReEchoCombat contracts")
    for forbidden_module in ("ReEcho", "ReEchoWeapons", "ReEchoAudio", "UMG", "Paper2D"):
        if f'"{forbidden_module}"' in enemies_build_text:
            fail(f"ReEchoEnemies must not depend on {forbidden_module}")

    enemies_forbidden_prefixes = (
        "Data/",
        "Graybox/",
        "Player/",
        "Presentation/",
        "Recording/",
        "Run/",
        "UI/",
        "Weapons/",
    )
    enemies_forbidden_logic_tokens = (
        "FindComponentByClass",
        "TActorIterator",
        "AReEchoGameMode",
        "AReEchoEnemyActor",
        "AReEchoPlayerPawn",
        "ApplyFinalDamage",
        "ReceiveGrayboxDamage",
        "ReceiveElementalDamage",
        "ReEchoGameplayEffects::ApplyDamage",
        "/Game/",
    )
    for path in enemies_root.rglob("*"):
        if path.suffix not in {".h", ".cpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        for include in include_pattern.findall(text_value):
            if include.startswith(enemies_forbidden_prefixes) or include in {"ReEcho.h", "ReEchoAudio.h"}:
                fail(f"{rel(path)} has forbidden main/presentation module include: {include}")
        for token in enemies_forbidden_logic_tokens:
            if token in text_value:
                fail(f"{rel(path)} violates the explicit Enemy logic boundary with token: {token}")

    enemies_modules = [module for module in descriptor.get("Modules", []) if module.get("Name") == "ReEchoEnemies"]
    if len(enemies_modules) != 1 or enemies_modules[0].get("Type") != "Runtime":
        fail("ReEcho.uproject must declare exactly one ReEchoEnemies Runtime module")

    enemy_host_header_path = ROOT / "Source" / "ReEcho" / "Public" / "Graybox" / "ReEchoEnemyActor.h"
    enemy_host_cpp_path = ROOT / "Source" / "ReEcho" / "Private" / "Graybox" / "ReEchoEnemyActor.cpp"
    enemy_host_header = enemy_host_header_path.read_text(encoding="utf-8")
    enemy_host_cpp = enemy_host_cpp_path.read_text(encoding="utf-8")
    legacy_enemy_host_state = (
        "float MoveSpeed =",
        "float ContactDamage =",
        "float AttackInterval =",
        "float AttackCooldown =",
        "float FuseRemaining =",
        "bool bBomberFuseActive",
        "float HitReactionRemaining =",
        "FVector KnockbackVelocity =",
        "float AttackVisualRemaining =",
        "float DeathVisualRemaining =",
        "int64 AttackSequence =",
    )
    for token in legacy_enemy_host_state:
        if token in enemy_host_header:
            fail(f"{rel(enemy_host_header_path)} retains duplicate Enemy logic/presentation state: {token}")
    if '"/Game/' in enemy_host_cpp:
        fail(f"{rel(enemy_host_cpp_path)} must not own enemy presentation asset paths")

    enemy_presentation_root = ROOT / "Source" / "ReEcho" / "Private" / "Presentation" / "Enemy"
    presentation_gameplay_tokens = (
        "ReEchoHitResolver",
        "ReceiveGrayboxDamage",
        "ReceiveElementalDamage",
        "NotifyHurt(",
        "NotifyDeath(",
        "RestoreSnapshot(",
        "AddActorWorldOffset",
        "SetActorLocation",
        "SetActorEnableCollision",
        "SetLifeSpan",
    )
    for path in enemy_presentation_root.rglob("*.cpp"):
        text_value = path.read_text(encoding="utf-8")
        for token in presentation_gameplay_tokens:
            if token in text_value:
                fail(f"{rel(path)} writes gameplay from enemy presentation with token: {token}")

    game_mode_path = ROOT / "Source" / "ReEcho" / "Private" / "ReEchoGameMode.cpp"
    if "TActorIterator<AReEchoEnemyActor>" in game_mode_path.read_text(encoding="utf-8"):
        fail("AReEchoGameMode must use the single EnemyRoster instead of scanning concrete enemy actors")


def validate_prebuilt_editor() -> None:
    tool = ROOT / "scripts" / "ue" / "prebuilt_editor.py"
    if not tool.is_file():
        fail("missing prebuilt Editor bundle tool")
    build_script = ROOT / "scripts" / "ue" / "Build-Editor.ps1"
    build_text = build_script.read_text(encoding="utf-8") if build_script.is_file() else ""
    if "[switch]$FullRebuild" not in build_text or "$buildArguments += '-Rebuild'" not in build_text:
        fail("Build-Editor.ps1 must expose the full-rebuild publication gate")
    result = subprocess.run(
        [sys.executable, str(tool), "check"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        fail("prebuilt Editor bundle check failed:\n" + result.stdout.strip())

    manifest_path = ROOT / "Binaries" / "Win64" / "ReEchoEditor.prebuilt.json"
    manifest = load_json(manifest_path)
    allowed = {
        "Binaries/Win64/ReEchoEditor.prebuilt.json",
        *(f"Binaries/Win64/{name}" for name in manifest.get("binaries", {})),
    }
    tracked_result = subprocess.run(
        ["git", "ls-files", "--cached", "Binaries"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    if tracked_result.returncode == 0:
        tracked = {line.strip().replace("\\", "/") for line in tracked_result.stdout.splitlines() if line.strip()}
        unexpected = sorted(tracked - allowed)
        missing = sorted(allowed - tracked)
        if unexpected:
            fail(f"tracked generated products exceed prebuilt allowlist: {', '.join(unexpected)}")
        if missing:
            fail(f"prebuilt Editor files are not staged/tracked: {', '.join(missing)}")


def validate_xlsx_authoring_sync() -> None:
    sync_script = ROOT / "scripts" / "data" / "sync_xlsx_to_csv.py"
    if not sync_script.is_file():
        fail("missing XLSX authoring sync script")
    result = subprocess.run(
        [sys.executable, str(sync_script), "--check"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        fail("XLSX authoring sync check failed:\n" + result.stdout.strip())


def main() -> int:
    validate_no_legacy_data_json()
    validate_csv_schema()
    validate_csv_package(DATA)
    with tempfile.TemporaryDirectory(prefix="reecho_csv_valid_alt_") as temp:
        validate_csv_package(assemble_fixture_package(DATA / "TestFixtures" / "CsvRuntime" / "ValidAlt", Path(temp)))
    with tempfile.TemporaryDirectory(prefix="reecho_csv_reaction_changed_") as temp:
        validate_csv_package(
            assemble_fixture_package(DATA / "TestFixtures" / "CsvRuntime" / "ReactionValueChanged", Path(temp))
        )
    with tempfile.TemporaryDirectory(prefix="reecho_csv_echo_efficiency_") as temp:
        validate_csv_package(
            assemble_fixture_package(DATA / "TestFixtures" / "CsvRuntime" / "EchoEfficiencyEnabled", Path(temp))
        )
    for name, token in {
        "DuplicateId": "duplicate id",
        "MissingRequired": "required value",
        "UnknownReference": "unknown reference",
        "UnknownWeaponReference": "DefaultWeaponId",
        "UnknownBehavior": "behavior id",
        "UnknownEffectKind": "effect kind",
        "IllegalRange": "exceeds",
        "UnsupportedVersion": "SchemaVersion",
        "DuplicateCardEffectOrder": "duplicate CardId/Order",
        "UnsupportedCardEffectTrigger": "Trigger",
        "UnknownCardEffectTarget": "Target",
        "InvalidCardEffectBehaviorPair": "EffectKind/BehaviorId",
        "UnknownFormulaId": "FormulaId",
        "DuplicateReactionPair": "duplicate ordered pair",
        "UnsupportedCanCrit": "CanCrit",
        "DuplicateWeaponInputSlot": "four-weapon input slot mapping",
        "InvalidWeaponPartEffectBehaviorPair": "EffectKind/BehaviorId",
    }.items():
        expect_fixture_failure(name, token)
    validate_build_dependencies()
    validate_combat_module_boundaries()
    validate_prebuilt_editor()
    validate_xlsx_authoring_sync()
    validate_workflow()

    print("[PASS] retired Content/Data gameplay JSON files are absent; production authority remains XLSX/CSV")
    print("[PASS] CSV schema, production character/build/element/weapon tables, fixtures, IDs, references, behavior/effect/formula/attack-pattern allowlists, UTF-8 and staging deps")
    print("[PASS] XLSX authoring workbook check matches generated production CSV bytes")
    print("[PASS] workflow memory, token guards, and Unreal project descriptor present")
    print("Evidence level: static verified only (no UHT/UBT/PIE claim)")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValidationError as exc:
        print(f"[FAIL] {exc}")
        sys.exit(1)
