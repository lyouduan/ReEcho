#include "Diagnostics/ReEchoBuildTrace.h"

#include "ReEcho.h"
#include "GameFramework/Actor.h"
#include "Misc/Crc.h"

namespace
{
FString EscapeTraceValue(FString Value)
{
	Value.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	Value.ReplaceInline(TEXT("|"), TEXT("\\|"));
	Value.ReplaceInline(TEXT(";"), TEXT("\\;"));
	Value.ReplaceInline(TEXT("="), TEXT("\\="));
	Value.ReplaceInline(TEXT(","), TEXT("\\,"));
	Value.ReplaceInline(TEXT("["), TEXT("\\["));
	Value.ReplaceInline(TEXT("]"), TEXT("\\]"));
	Value.ReplaceInline(TEXT("\r"), TEXT("\\r"));
	Value.ReplaceInline(TEXT("\n"), TEXT("\\n"));
	return Value;
}

FString NameValue(const FName Value)
{
	return EscapeTraceValue(Value.IsNone() ? TEXT("None") : Value.ToString());
}

FString FormatFloat(const float Value)
{
	return FString::Printf(TEXT("%.6f"), Value);
}

FString DescribeStats(const FReEchoStatBlock& Stats)
{
	return FString::Printf(
	    TEXT("hp=%s/%s,physical=%s,elemental=%s,block=%d,attackSpeed=%s,moveSpeed=%s,critRate=%s,"
	         "critEffect=%s,echoEfficiency=%s,reactionEfficiency=%s,role=%s,projectiles=%d,weaponSize=%s,"
	         "echoCount=%d,shopDiscount=%s,characterSize=%s,concentration=%s,pathAffinity=%s"),
	    *FormatFloat(Stats.HpPoint),
	    *FormatFloat(Stats.HpMax),
	    *FormatFloat(Stats.PhysicalAttack),
	    *FormatFloat(Stats.ElementalAttack),
	    Stats.Block,
	    *FormatFloat(Stats.AttackSpeed),
	    *FormatFloat(Stats.MovementSpeed),
	    *FormatFloat(Stats.CriticalRate),
	    *FormatFloat(Stats.CriticalEffect),
	    *FormatFloat(Stats.EchoEfficiency),
	    *FormatFloat(Stats.ReactionEfficiency),
	    *NameValue(Stats.RoleId),
	    Stats.ProjectileCount,
	    *FormatFloat(Stats.WeaponSize),
	    Stats.EchoCount,
	    *FormatFloat(Stats.ShopDiscount),
	    *FormatFloat(Stats.CharacterSize),
	    *FormatFloat(Stats.Concentration),
	    *FormatFloat(Stats.PathAffinity));
}

FString DescribeRules(const TMap<FName, FString>& Rules)
{
	TArray<FName> Keys;
	Rules.GetKeys(Keys);
	Keys.Sort([](const FName Left, const FName Right)
	{
		return Left.ToString() < Right.ToString();
	});

	TArray<FString> Entries;
	Entries.Reserve(Keys.Num());
	for (const FName Key : Keys)
	{
		Entries.Add(FString::Printf(TEXT("%s=%s"), *NameValue(Key), *EscapeTraceValue(Rules.FindChecked(Key))));
	}
	return Entries.IsEmpty() ? TEXT("-") : FString::Join(Entries, TEXT(","));
}

FString DescribeRunes(const TArray<FReEchoEquippedPartSnapshot>& EquippedParts)
{
	TArray<FString> Entries;
	Entries.Reserve(EquippedParts.Num());
	for (const FReEchoEquippedPartSnapshot& Equipped : EquippedParts)
	{
		Entries.Add(FString::Printf(TEXT("%s:%s"), *NameValue(Equipped.SlotTypeId), *NameValue(Equipped.PartId)));
	}
	Entries.Sort();
	return Entries.IsEmpty() ? TEXT("-") : FString::Join(Entries, TEXT(","));
}

FString DescribeCards(const TArray<FName>& OwnedCardIds)
{
	TMap<FName, int32> Stacks;
	for (const FName CardId : OwnedCardIds)
	{
		++Stacks.FindOrAdd(CardId);
	}
	TArray<FName> CardIds;
	Stacks.GetKeys(CardIds);
	CardIds.Sort([](const FName Left, const FName Right)
	{
		return Left.ToString() < Right.ToString();
	});

	TArray<FString> Entries;
	Entries.Reserve(CardIds.Num());
	for (const FName CardId : CardIds)
	{
		Entries.Add(FString::Printf(TEXT("%sx%d"), *NameValue(CardId), Stacks.FindChecked(CardId)));
	}
	return Entries.IsEmpty() ? TEXT("-") : FString::Join(Entries, TEXT(","));
}

FString DescribeNameArray(TArray<FName> Values)
{
	Values.Sort([](const FName Left, const FName Right)
	{
		return Left.ToString() < Right.ToString();
	});
	TArray<FString> Entries;
	Entries.Reserve(Values.Num());
	for (const FName Value : Values)
	{
		Entries.Add(NameValue(Value));
	}
	return Entries.IsEmpty() ? TEXT("-") : FString::Join(Entries, TEXT(","));
}

FString DescribeNameCountMap(const TMap<FName, int32>& Values)
{
	TArray<FName> Keys;
	Values.GetKeys(Keys);
	Keys.Sort([](const FName Left, const FName Right)
	{
		return Left.ToString() < Right.ToString();
	});
	TArray<FString> Entries;
	Entries.Reserve(Keys.Num());
	for (const FName Key : Keys)
	{
		Entries.Add(FString::Printf(TEXT("%s=%d"), *NameValue(Key), Values.FindChecked(Key)));
	}
	return Entries.IsEmpty() ? TEXT("-") : FString::Join(Entries, TEXT(","));
}

FString DescribeRuntime(const FReEchoCardRuntimeState& Runtime)
{
	TArray<int32> ConductSpawnIndices = Runtime.ConductAffectedSpawnIndices;
	ConductSpawnIndices.Sort();
	TArray<FString> ConductEntries;
	ConductEntries.Reserve(ConductSpawnIndices.Num());
	for (const int32 SpawnIndex : ConductSpawnIndices)
	{
		ConductEntries.Add(FString::FromInt(SpawnIndex));
	}

	return FString::Printf(
	    TEXT("random=%d,activeEncounter=%d,prevented=%d,huntKills=%d,reactions=%d,echoKillProgress=%d,"
	         "playerKillProgress=%d,encounterKills=%d,playerKillsByEnemy={%s},distinctReactions=[%s],recordedReaction=%s,"
	         "debt=%d,masteredWeapons=[%s],playerDamageMultiplier=%s,echoDamageMultiplier=%s,conductCount=%d,"
	         "conductSpawns=[%s],conductPlayerMultiplier=%s,echoTrinityGranted=%s"),
	    Runtime.RandomSequence,
	    Runtime.ActiveEncounterIndex,
	    Runtime.PreventedDamageCount,
	    Runtime.HuntKillCount,
	    Runtime.ReactionCount,
	    Runtime.EchoKillProgress,
	    Runtime.PlayerKillProgress,
	    Runtime.EncounterKillCount,
	    *DescribeNameCountMap(Runtime.PlayerKillCountByEnemyId),
	    *DescribeNameArray(Runtime.DistinctReactionIds),
	    *NameValue(Runtime.RecordedReactionId),
	    Runtime.TimeShardDebt,
	    *DescribeNameArray(Runtime.MasteredWeaponIds),
	    *FormatFloat(Runtime.PlayerDamageMultiplier),
	    *FormatFloat(Runtime.EchoDamageMultiplier),
	    Runtime.ConductAffectedCount,
	    ConductEntries.IsEmpty() ? TEXT("-") : *FString::Join(ConductEntries, TEXT(",")),
	    *FormatFloat(Runtime.ConductPlayerDamageMultiplier),
	    *FormatFloat(Runtime.EchoTrinityEfficiencyGranted));
}
} // namespace

namespace ReEchoBuildTrace
{
FString BuildCanonical(const FReEchoBuildSnapshot& Build)
{
	return FString::Printf(
	    TEXT("character=%s|weapon=%s|weaponRevision=%d|weaponDomain=%s|hasEquipmentBase=%d|equipmentBaseStats={%s}|"
	         "equipmentBaseRules={%s}|runes=[%s]|cardDomain=%s|cards=[%s]|stats={%s}|rules={%s}|runtime={%s}"),
	    *NameValue(Build.CharacterId),
	    *NameValue(Build.WeaponId),
	    Build.WeaponDataRevision,
	    *EscapeTraceValue(Build.WeaponDomainRevision),
	    Build.bHasEquipmentBase ? 1 : 0,
	    *DescribeStats(Build.EquipmentBaseStats),
	    *DescribeRules(Build.EquipmentBaseRuleFlags),
	    *DescribeRunes(Build.EquippedParts),
	    *EscapeTraceValue(Build.CardState.DomainRevision),
	    *DescribeCards(Build.CardState.OwnedCardIds),
	    *DescribeStats(Build.Stats),
	    *DescribeRules(Build.RuleFlags),
	    *DescribeRuntime(Build.CardState.Runtime));
}

FString ComputeFingerprint(const FReEchoBuildSnapshot& Build)
{
	return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*BuildCanonical(Build)));
}

FString BuildSummary(const FReEchoBuildSnapshot& Build)
{
	return FString::Printf(TEXT("fingerprint=%s %s"), *ComputeFingerprint(Build), *BuildCanonical(Build));
}

void LogSnapshot(const TCHAR* Event,
                 const int32 EncounterIndex,
                 const EReEchoRunPhase Phase,
                 const FReEchoBuildSnapshot& Build,
                 const FString& Detail)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[BuildSnapshotTrace] event=%s encounter=%d phase=%d detail=%s %s"),
	       Event ? Event : TEXT("Unknown"),
	       EncounterIndex,
	       static_cast<int32>(Phase),
	       Detail.IsEmpty() ? TEXT("-") : *EscapeTraceValue(Detail),
	       *BuildSummary(Build));
#endif
}

void LogAttackCommit(const AActor* Source,
                     const AActor* Target,
                     const FVector& Origin,
                     const FVector& Direction,
                     const FReEchoBuildSnapshot& Build,
                     const FReEchoWeaponAttackCommit& Commit)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[BuildCommitTrace] source=%s sourceClass=%s target=%s origin=(%.3f,%.3f,%.3f) "
	            "direction=(%.6f,%.6f,%.6f) weapon=%s sequence=%lld fingerprint=%s pattern=%s step=%s[%d] "
	            "carrier=%d rawDamage=%.6f critical=%d element=%d projectiles=%d"),
	       *GetNameSafe(Source),
	       Source ? *GetNameSafe(Source->GetClass()) : TEXT("None"),
	       *GetNameSafe(Target),
	       Origin.X,
	       Origin.Y,
	       Origin.Z,
	       Direction.X,
	       Direction.Y,
	       Direction.Z,
	       *Commit.WeaponId.ToString(),
	       static_cast<long long>(Commit.Attack.Sequence),
	       *ComputeFingerprint(Build),
	       *Commit.AttackPatternId.ToString(),
	       *Commit.AttackStepId.ToString(),
	       Commit.StepIndex,
	       static_cast<int32>(Commit.Carrier),
	       Commit.RawDamage,
	       Commit.bCritical ? 1 : 0,
	       static_cast<int32>(Commit.Element),
	       Commit.ProjectileCount);
#endif
}
} // namespace ReEchoBuildTrace
