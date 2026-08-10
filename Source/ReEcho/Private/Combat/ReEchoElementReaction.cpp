#include "Combat/ReEchoElementReaction.h"

#include "AbilitySystem/ReEchoGameplayEffects.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"

namespace ReEchoElementReaction
{
namespace
{
constexpr const TCHAR* ElementImmunityStatusId = TEXT("Z_Elemental_Immunity");
constexpr const TCHAR* BurnStatusId = TEXT("Z_Burn");
constexpr const TCHAR* StatusNone = TEXT("None");
constexpr float BurnTickIntervalSeconds = 1.0f;

const FReEchoCsvElementRow* FindElementRow(const EReEchoElement Element)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return Snapshot.IsValid() ? Snapshot->FindElement(Element) : nullptr;
}

uint8 ParseHexPair(const FString& Text, const int32 Index)
{
	const FString Pair = Text.Mid(Index, 2);
	return static_cast<uint8>(FCString::Strtoi(*Pair, nullptr, 16));
}

FLinearColor ParseColorHex(const FString& ColorHex)
{
	if (!ColorHex.StartsWith(TEXT("#")) || ColorHex.Len() != 7)
	{
		return FLinearColor::White;
	}
	const FColor Color(ParseHexPair(ColorHex, 1), ParseHexPair(ColorHex, 3), ParseHexPair(ColorHex, 5));
	return FLinearColor::FromSRGBColor(Color);
}

bool IsAttachmentRole(const FReEchoCsvElementRow& Element)
{
	return Element.Role == EReEchoElementRole::Attachment;
}

EReEchoElement ParseElementId(const FName ElementId)
{
	if (ElementId == TEXT("Flame"))
	{
		return EReEchoElement::Flame;
	}
	if (ElementId == TEXT("Lightning"))
	{
		return EReEchoElement::Lightning;
	}
	if (ElementId == TEXT("Grass"))
	{
		return EReEchoElement::Grass;
	}
	if (ElementId == TEXT("Water"))
	{
		return EReEchoElement::Water;
	}
	return EReEchoElement::None;
}

float GetEnabledStatusDuration(const FReEchoCsvDataSnapshot& Snapshot, const FName StatusId)
{
	const FReEchoCsvStatusRow* Status = Snapshot.FindStatus(StatusId);
	return Status && Status->bEnabled ? Status->DurationSeconds : 0.0f;
}

float GetFinalReactionMultiplier(const FReEchoElementHitResult& Result)
{
	return Result.bAppliedEnhancement ? FMath::Max(1.0f, Result.EnhancementMultiplier) : 1.0f;
}

float GetEchoMultiplier(const FReEchoElementHitResult& Result, const FReEchoElementHitContext& Context)
{
	return Result.bAffectedByEchoEfficiency ? FMath::Max(0.0f, Context.SourceEchoEfficiency) : 1.0f;
}

float CalculateElementAttackScaleDamage(const FReEchoCsvReactionRow& Reaction,
                                        const FReEchoElementHitResult& Result,
                                        const FReEchoElementHitContext& Context)
{
	return FMath::Max(0.0f, Context.SourceElementalAttack) * Reaction.DamageMultiplier *
	       FMath::Max(0.0f, Context.ReactionEfficiency) * GetFinalReactionMultiplier(Result) *
	       GetEchoMultiplier(Result, Context);
}

float CalculateConductDamage(const FReEchoCsvReactionRow& Reaction,
                             const FReEchoElementHitResult& Result,
                             const FReEchoElementHitContext& Context)
{
	const float ElementalAttack = FMath::Max(0.0f, Context.SourceElementalAttack);
	return (ElementalAttack + 2.0f) * FMath::Max(0.0f, Context.ReactionEfficiency) * Reaction.DamageIncrease *
	       GetFinalReactionMultiplier(Result) * GetEchoMultiplier(Result, Context);
}

float CalculateElementAttackSquaredDamage(const FReEchoCsvReactionRow& Reaction,
                                          const FReEchoElementHitResult& Result,
                                          const FReEchoElementHitContext& Context)
{
	const float ElementalAttack = FMath::Max(0.0f, Context.SourceElementalAttack);
	return ElementalAttack * ElementalAttack * Reaction.DamageIncrease * FMath::Max(0.0f, Context.ReactionEfficiency) *
	       GetFinalReactionMultiplier(Result) * GetEchoMultiplier(Result, Context);
}

void ApplyElementalImmunity(AReEchoEnemyActor& Target,
                            const FReEchoCsvDataSnapshot& Snapshot,
                            const float CurrentTimeSeconds)
{
	const float ImmunityDuration = GetEnabledStatusDuration(Snapshot, ElementImmunityStatusId);
	if (ImmunityDuration <= 0.0f || CurrentTimeSeconds < 0.0f)
	{
		return;
	}
	FReEchoElementState& State = Target.EditElementState();
	State.ImmunityUntil = CurrentTimeSeconds + ImmunityDuration;
	State.ActiveStatusUntilSeconds.Add(FName(ElementImmunityStatusId), State.ImmunityUntil);
}

bool ShouldApplyElementalImmunity(const FName ReactionBehaviorId)
{
	return ReactionBehaviorId == TEXT("Reaction.Burn") || ReactionBehaviorId == TEXT("Reaction.Vaporize") ||
	       ReactionBehaviorId == TEXT("Reaction.Conduct");
}

void ApplyStatus(AReEchoEnemyActor& Target,
                 const FName StatusId,
                 const float OverrideDurationSeconds,
                 const FReEchoCsvDataSnapshot& Snapshot,
                 const float CurrentTimeSeconds)
{
	if (StatusId == NAME_None || CurrentTimeSeconds < 0.0f)
	{
		return;
	}
	const float Duration =
	    OverrideDurationSeconds > 0.0f ? OverrideDurationSeconds : GetEnabledStatusDuration(Snapshot, StatusId);
	if (Duration <= 0.0f)
	{
		return;
	}
	Target.EditElementState().ActiveStatusUntilSeconds.Add(StatusId, CurrentTimeSeconds + Duration);
}

bool IsElementalImmune(const AReEchoEnemyActor& Target, const float CurrentTimeSeconds)
{
	return CurrentTimeSeconds >= 0.0f && Target.GetElementState().ImmunityUntil > CurrentTimeSeconds;
}

bool SortEnemyStable(const AReEchoEnemyActor& Left, const AReEchoEnemyActor& Right, const FVector& Origin)
{
	const float LeftDistance = FVector::DistSquared2D(Left.GetActorLocation(), Origin);
	const float RightDistance = FVector::DistSquared2D(Right.GetActorLocation(), Origin);
	if (!FMath::IsNearlyEqual(LeftDistance, RightDistance))
	{
		return LeftDistance < RightDistance;
	}
	const FVector LeftLocation = Left.GetActorLocation();
	const FVector RightLocation = Right.GetActorLocation();
	if (!FMath::IsNearlyEqual(LeftLocation.X, RightLocation.X))
	{
		return LeftLocation.X < RightLocation.X;
	}
	if (!FMath::IsNearlyEqual(LeftLocation.Y, RightLocation.Y))
	{
		return LeftLocation.Y < RightLocation.Y;
	}
	if (!FMath::IsNearlyEqual(LeftLocation.Z, RightLocation.Z))
	{
		return LeftLocation.Z < RightLocation.Z;
	}
	return Left.GetFName().LexicalLess(Right.GetFName());
}

TArray<AReEchoEnemyActor*> GetAliveEnemiesInRadius(UWorld& World, const FVector& Origin, const float RadiusCm)
{
	TArray<AReEchoEnemyActor*> Targets;
	const float RadiusSquared = FMath::Square(FMath::Max(0.0f, RadiusCm));
	for (TActorIterator<AReEchoEnemyActor> It(&World); It; ++It)
	{
		AReEchoEnemyActor* Candidate = *It;
		if (!Candidate || !Candidate->IsAlive())
		{
			continue;
		}
		if (FVector::DistSquared2D(Candidate->GetActorLocation(), Origin) <= RadiusSquared)
		{
			Targets.Add(Candidate);
		}
	}
	Targets.Sort(
	    [&Origin](const AReEchoEnemyActor& Left, const AReEchoEnemyActor& Right)
	    {
		    return SortEnemyStable(Left, Right, Origin);
	    });
	return Targets;
}

int32 CountRemainingBurnTicks(const float NextTickTimeSeconds, const float EndTimeSeconds)
{
	if (NextTickTimeSeconds < 0.0f || EndTimeSeconds < 0.0f ||
	    NextTickTimeSeconds > EndTimeSeconds + KINDA_SMALL_NUMBER)
	{
		return 0;
	}
	return FMath::Max(0, FMath::FloorToInt((EndTimeSeconds - NextTickTimeSeconds) / BurnTickIntervalSeconds) + 1);
}

void ClearBurnState(FReEchoElementState& State, const bool bRemoveStatus)
{
	State.bBurnActive = false;
	State.BurnTickDamage = 0.0f;
	State.BurnNextTickTimeSeconds = 0.0f;
	State.BurnSourceLocation = FVector::ZeroVector;
	State.BurnSourceActor.Reset();
	if (bRemoveStatus)
	{
		State.ActiveStatusUntilSeconds.Remove(FName(BurnStatusId));
	}
}

int32 TickBurnStatus(AReEchoEnemyActor& Target, const float CurrentTimeSeconds)
{
	if (CurrentTimeSeconds < 0.0f || !Target.IsAlive())
	{
		return 0;
	}

	FReEchoElementState& State = Target.EditElementState();
	float* BurnUntil = State.ActiveStatusUntilSeconds.Find(FName(BurnStatusId));
	if (!State.bBurnActive || !BurnUntil || State.BurnTickDamage <= 0.0f || State.BurnNextTickTimeSeconds < 0.0f)
	{
		ClearBurnState(State, BurnUntil != nullptr && CurrentTimeSeconds > *BurnUntil + KINDA_SMALL_NUMBER);
		return 0;
	}

	int32 AppliedTicks = 0;
	while (Target.IsAlive() && State.BurnNextTickTimeSeconds <= CurrentTimeSeconds + KINDA_SMALL_NUMBER &&
	       State.BurnNextTickTimeSeconds <= *BurnUntil + KINDA_SMALL_NUMBER)
	{
		Target.ReceiveGrayboxDamage(State.BurnTickDamage,
		                            State.BurnSourceLocation,
		                            State.BurnSourceActor.Get(),
		                            GetElementColor(EReEchoElement::Flame));
		State.BurnNextTickTimeSeconds += BurnTickIntervalSeconds;
		++AppliedTicks;
	}

	if (State.BurnNextTickTimeSeconds > *BurnUntil + KINDA_SMALL_NUMBER || CurrentTimeSeconds > *BurnUntil)
	{
		ClearBurnState(State, true);
	}
	return AppliedTicks;
}

void StartOrRefreshBurn(AReEchoEnemyActor& Target,
                        const FReEchoElementHitResult& Result,
                        const float TickDamage,
                        const FReEchoElementHitContext& Context,
                        const FReEchoCsvDataSnapshot& Snapshot,
                        const float CurrentTimeSeconds,
                        FReEchoElementExecutionResult& ExecutionResult)
{
	if (CurrentTimeSeconds < 0.0f || TickDamage <= 0.0f)
	{
		return;
	}
	const float Duration = Result.StatusDurationSeconds > 0.0f ? Result.StatusDurationSeconds
	                                                           : GetEnabledStatusDuration(Snapshot, BurnStatusId);
	if (Duration <= 0.0f)
	{
		return;
	}

	FReEchoElementState& State = Target.EditElementState();
	const float EndTimeSeconds = CurrentTimeSeconds + Duration;
	const float ExistingEndTime = State.ActiveStatusUntilSeconds.FindRef(FName(BurnStatusId));
	if (State.bBurnActive && State.BurnNextTickTimeSeconds <= CurrentTimeSeconds + KINDA_SMALL_NUMBER &&
	    State.BurnNextTickTimeSeconds <= ExistingEndTime + KINDA_SMALL_NUMBER)
	{
		TickBurnStatus(Target, CurrentTimeSeconds);
	}
	if (!Target.IsAlive())
	{
		return;
	}

	const float PostTickExistingEndTime = State.ActiveStatusUntilSeconds.FindRef(FName(BurnStatusId));
	const bool bRefreshOnlyActive = State.bBurnActive && PostTickExistingEndTime > CurrentTimeSeconds &&
	                                State.BurnNextTickTimeSeconds > CurrentTimeSeconds;
	State.ActiveStatusUntilSeconds.Add(FName(BurnStatusId), EndTimeSeconds);
	State.bBurnActive = true;
	if (!bRefreshOnlyActive)
	{
		State.BurnTickDamage = TickDamage;
		State.BurnNextTickTimeSeconds = CurrentTimeSeconds + BurnTickIntervalSeconds;
		State.BurnSourceLocation = Context.SourceLocation;
		State.BurnSourceActor = Context.SourceActor;
	}

	const int32 RemainingTicks = CountRemainingBurnTicks(State.BurnNextTickTimeSeconds, EndTimeSeconds);
	ExecutionResult.DotTicksScheduled = RemainingTicks;
	for (int32 TickIndex = 0; TickIndex < RemainingTicks; ++TickIndex)
	{
		ExecutionResult.DotTickDelaySeconds.Add(State.BurnNextTickTimeSeconds +
		                                        static_cast<float>(TickIndex) * BurnTickIntervalSeconds -
		                                        CurrentTimeSeconds);
	}
}

bool AttachElementIfAllowed(AReEchoEnemyActor& Target, const EReEchoElement Element, const float CurrentTimeSeconds)
{
	if (IsElementalImmune(Target, CurrentTimeSeconds))
	{
		return false;
	}
	FReEchoElementState& State = Target.EditElementState();
	if (State.BlockedAttachment == Element)
	{
		return false;
	}
	State.Attached = Element;
	Target.RefreshElementAttachmentVisual();
	return true;
}

void ApplyDamageAndImmunity(AReEchoEnemyActor& Target,
                            const float Damage,
                            const FReEchoElementHitContext& Context,
                            const FLinearColor& DamageColor,
                            const FReEchoCsvDataSnapshot& Snapshot,
                            const float CurrentTimeSeconds,
                            FReEchoElementExecutionResult& ExecutionResult)
{
	ExecutionResult.ImmediateDamageApplied +=
	    Target.ReceiveGrayboxDamage(Damage, Context.SourceLocation, Context.SourceActor.Get(), DamageColor);
	ApplyElementalImmunity(Target, Snapshot, CurrentTimeSeconds);
	Target.RefreshElementAttachmentVisual();
	ExecutionResult.AffectedTargets.Add(&Target);
}

TArray<AReEchoEnemyActor*> GetConductChainTargets(AReEchoEnemyActor& PrimaryTarget, UWorld& World, const float RadiusCm)
{
	TArray<AReEchoEnemyActor*> Result;
	TSet<AReEchoEnemyActor*> Visited;
	TArray<AReEchoEnemyActor*> Queue;
	Queue.Add(&PrimaryTarget);
	Visited.Add(&PrimaryTarget);

	while (!Queue.IsEmpty())
	{
		AReEchoEnemyActor* Current = Queue[0];
		Queue.RemoveAt(0, 1, EAllowShrinking::No);
		Result.Add(Current);

		TArray<AReEchoEnemyActor*> Neighbors = GetAliveEnemiesInRadius(World, Current->GetActorLocation(), RadiusCm);
		for (AReEchoEnemyActor* Neighbor : Neighbors)
		{
			if (!Neighbor || Visited.Contains(Neighbor))
			{
				continue;
			}
			if (Neighbor != &PrimaryTarget && Neighbor->GetAttachedElement() != EReEchoElement::Water)
			{
				continue;
			}
			Visited.Add(Neighbor);
			Queue.Add(Neighbor);
		}
	}
	return Result;
}
}

bool IsCombatElement(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	return Row && Row->bEnabled;
}

bool IsDoubleDamagePair(const EReEchoElement First, const EReEchoElement Second)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvElementRow* FirstRow = Snapshot.IsValid() ? Snapshot->FindElement(First) : nullptr;
	const FReEchoCsvElementRow* SecondRow = Snapshot.IsValid() ? Snapshot->FindElement(Second) : nullptr;
	if (!Snapshot.IsValid() || !FirstRow || !SecondRow || First == Second)
	{
		return false;
	}
	return Snapshot->FindReaction(FirstRow->Id, SecondRow->Id) != nullptr ||
	       Snapshot->FindReaction(SecondRow->Id, FirstRow->Id) != nullptr;
}

FReEchoElementHitResult ResolveHit(FReEchoElementState& State,
                                   const EReEchoElement IncomingElement,
                                   const float BaseDamage,
                                   const float ReactionEfficiency,
                                   const float CurrentTimeSeconds)
{
	FReEchoElementHitResult Result;
	Result.Damage = FMath::Max(0.0f, BaseDamage);
	Result.PreviousElement = State.Attached;
	Result.IncomingElement = IncomingElement;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvElementRow* IncomingRow = Snapshot.IsValid() ? Snapshot->FindElement(IncomingElement) : nullptr;
	if (!Snapshot.IsValid() || !IncomingRow || !IncomingRow->bEnabled)
	{
		return Result;
	}

	if (CurrentTimeSeconds >= 0.0f && State.ImmunityUntil > CurrentTimeSeconds)
	{
		Result.bBlockedByImmunity = true;
		return Result;
	}
	if (State.BlockedAttachment == IncomingElement)
	{
		Result.bBlockedByImmunity = true;
		return Result;
	}

	const FReEchoCsvElementRow* AttachedRow = Snapshot->FindElement(State.Attached);
	const FReEchoCsvReactionRow* Reaction =
	    AttachedRow && AttachedRow->bEnabled ? Snapshot->FindReaction(IncomingRow->Id, AttachedRow->Id) : nullptr;
	if (Reaction)
	{
		const bool bIsEnhancementReaction = Reaction->FormulaId == TEXT("Element.EnhanceNextReaction");
		Result.bTriggeredReaction = true;
		Result.ReactionId = Reaction->Id;
		Result.ReactionBehaviorId = Reaction->BehaviorId;
		Result.FormulaId = Reaction->FormulaId;
		Result.RadiusCm = Reaction->RadiusCm;
		Result.AppliedStatusId = Reaction->StatusId == StatusNone ? NAME_None : Reaction->StatusId;
		Result.StatusDurationSeconds = Reaction->StatusDurationSeconds;
		Result.EnhancementMultiplier = Reaction->EnhancementMultiplier;
		Result.bCanCrit = Reaction->bCanCrit;
		Result.bAffectedByEchoEfficiency = Reaction->bAffectedByEchoEfficiency;

		if (State.bEnhancedNextReaction && !bIsEnhancementReaction)
		{
			Result.bAppliedEnhancement = true;
			Result.EnhancementMultiplier = FMath::Max(1.0f, State.EnhancementMultiplier);
			State.bEnhancedNextReaction = false;
			State.EnhancementMultiplier = 1.0f;
			State.BlockedAttachment = EReEchoElement::None;
			Result.bClearedAttachmentBlock = true;
		}

		Result.Damage = 0.0f;
		if (bIsEnhancementReaction)
		{
			State.bEnhancedNextReaction = true;
			State.EnhancementMultiplier = FMath::Max(State.EnhancementMultiplier, Reaction->EnhancementMultiplier);
			State.BlockedAttachment = State.Attached;
		}
		else
		{
			Result.Multiplier = FMath::Max(0.0f, ReactionEfficiency) * GetFinalReactionMultiplier(Result);
		}

		if (Result.AppliedStatusId != NAME_None && CurrentTimeSeconds >= 0.0f)
		{
			const float Duration = Result.StatusDurationSeconds > 0.0f
			                           ? Result.StatusDurationSeconds
			                           : GetEnabledStatusDuration(*Snapshot, Result.AppliedStatusId);
			State.ActiveStatusUntilSeconds.Add(Result.AppliedStatusId, CurrentTimeSeconds + Duration);
		}
		if (ShouldApplyElementalImmunity(Reaction->BehaviorId))
		{
			const float ImmunityDuration = GetEnabledStatusDuration(*Snapshot, ElementImmunityStatusId);
			if (ImmunityDuration > 0.0f)
			{
				Result.ImmunityDurationSeconds = ImmunityDuration;
				if (CurrentTimeSeconds >= 0.0f)
				{
					State.ImmunityUntil = CurrentTimeSeconds + ImmunityDuration;
					State.ActiveStatusUntilSeconds.Add(FName(ElementImmunityStatusId), State.ImmunityUntil);
				}
			}
		}
		if (Reaction->bClearsAttachment)
		{
			State.Attached = EReEchoElement::None;
		}
	}
	else if (IsAttachmentRole(*IncomingRow))
	{
		State.Attached = IncomingElement;
	}
	return Result;
}

FReEchoElementExecutionResult ApplyHitToWorld(AReEchoEnemyActor& Target,
                                              const EReEchoElement IncomingElement,
                                              const float BaseDamage,
                                              const FReEchoElementHitContext& Context)
{
	FReEchoElementExecutionResult ExecutionResult;
	UWorld* World = Target.GetWorld();
	const float CurrentTimeSeconds = World ? World->GetTimeSeconds() : -1.0f;
	ExecutionResult.Primary = ResolveHit(
	    Target.EditElementState(), IncomingElement, BaseDamage, Context.ReactionEfficiency, CurrentTimeSeconds);
	Target.RefreshElementAttachmentVisual();

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return ExecutionResult;
	}

	if (!ExecutionResult.Primary.bTriggeredReaction)
	{
		ExecutionResult.ImmediateDamageApplied = Target.ReceiveGrayboxDamage(
		    ExecutionResult.Primary.Damage,
		    Context.SourceLocation,
		    Context.SourceActor.Get(),
		    ExecutionResult.Primary.bBlockedByImmunity ? FLinearColor::White : GetElementColor(IncomingElement));
		ExecutionResult.AffectedTargets.Add(&Target);
		return ExecutionResult;
	}

	const FReEchoCsvReactionRow* Reaction = Snapshot->Reactions.Find(ExecutionResult.Primary.ReactionId);
	if (!Reaction)
	{
		return ExecutionResult;
	}

	if (Reaction->BehaviorId == TEXT("Reaction.Burn"))
	{
		const float TickDamage = CalculateElementAttackScaleDamage(*Reaction, ExecutionResult.Primary, Context);
		StartOrRefreshBurn(
		    Target, ExecutionResult.Primary, TickDamage, Context, *Snapshot, CurrentTimeSeconds, ExecutionResult);
		ApplyElementalImmunity(Target, *Snapshot, CurrentTimeSeconds);
		ExecutionResult.AffectedTargets.Add(&Target);
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Vaporize"))
	{
		ApplyStatus(Target,
		            ExecutionResult.Primary.AppliedStatusId,
		            ExecutionResult.Primary.StatusDurationSeconds,
		            *Snapshot,
		            CurrentTimeSeconds);
		const float Damage = CalculateElementAttackSquaredDamage(*Reaction, ExecutionResult.Primary, Context);
		ApplyDamageAndImmunity(Target,
		                       Damage,
		                       Context,
		                       FLinearColor(1.0f, 0.72f, 0.12f, 1.0f),
		                       *Snapshot,
		                       CurrentTimeSeconds,
		                       ExecutionResult);
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Growth") && World)
	{
		ApplyStatus(Target,
		            ExecutionResult.Primary.AppliedStatusId,
		            ExecutionResult.Primary.StatusDurationSeconds,
		            *Snapshot,
		            CurrentTimeSeconds);
		const float RadiusCm = Reaction->RadiusCm * FMath::Max(0.0f, Context.ReactionEfficiency);
		TArray<AReEchoEnemyActor*> Targets = GetAliveEnemiesInRadius(*World, Target.GetActorLocation(), RadiusCm);
		for (AReEchoEnemyActor* TargetInRadius : Targets)
		{
			if (AttachElementIfAllowed(*TargetInRadius, EReEchoElement::Grass, CurrentTimeSeconds))
			{
				ExecutionResult.AffectedTargets.Add(TargetInRadius);
			}
		}
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Conduct") && World)
	{
		ApplyStatus(Target,
		            ExecutionResult.Primary.AppliedStatusId,
		            ExecutionResult.Primary.StatusDurationSeconds,
		            *Snapshot,
		            CurrentTimeSeconds);
		const float Damage = CalculateConductDamage(*Reaction, ExecutionResult.Primary, Context);
		TArray<AReEchoEnemyActor*> ChainTargets = GetConductChainTargets(Target, *World, Reaction->RadiusCm);
		for (AReEchoEnemyActor* ChainTarget : ChainTargets)
		{
			ChainTarget->EditElementState().Attached = EReEchoElement::None;
			ApplyDamageAndImmunity(*ChainTarget,
			                       Damage,
			                       Context,
			                       FLinearColor(1.0f, 0.72f, 0.12f, 1.0f),
			                       *Snapshot,
			                       CurrentTimeSeconds,
			                       ExecutionResult);
		}
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Enhance"))
	{
		ApplyStatus(Target,
		            ExecutionResult.Primary.AppliedStatusId,
		            ExecutionResult.Primary.StatusDurationSeconds,
		            *Snapshot,
		            CurrentTimeSeconds);
		ExecutionResult.AffectedTargets.Add(&Target);
	}

	Target.RefreshElementAttachmentVisual();
	return ExecutionResult;
}

int32 TickElementStatuses(AReEchoEnemyActor& Target, const float CurrentTimeSeconds)
{
	return TickBurnStatus(Target, CurrentTimeSeconds);
}

FString GetElementLabel(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	if (Row)
	{
		return Row->VisualKey.ToString().ToUpper();
	}
	return TEXT("NONE");
}

FLinearColor GetElementColor(const EReEchoElement Element)
{
	const FReEchoCsvElementRow* Row = FindElementRow(Element);
	if (Row)
	{
		return ParseColorHex(Row->ColorHex);
	}
	return FLinearColor::White;
}

FName GetElementId(const EReEchoElement Element)
{
	switch (Element)
	{
		case EReEchoElement::Flame:
			return TEXT("Flame");
		case EReEchoElement::Lightning:
			return TEXT("Lightning");
		case EReEchoElement::Grass:
			return TEXT("Grass");
		case EReEchoElement::Water:
			return TEXT("Water");
		default:
			return NAME_None;
	}
}
}
