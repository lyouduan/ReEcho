#include "Combat/ReEchoHitResolver.h"

#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

class FReEchoElementResolverAccess
{
public:
	static FReEchoElementState& Edit(UReEchoCombatantComponent& Combatant)
	{
		return Combatant.ElementState;
	}
};

namespace
{
constexpr const TCHAR* ElementImmunityStatusId = TEXT("Z_Elemental_Immunity");
constexpr const TCHAR* BurnStatusId = TEXT("Z_Burn");
constexpr float BurnTickIntervalSeconds = 1.0f;

UReEchoCombatantComponent* GetCombatant(AActor& Actor)
{
	IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(&Actor);
	return Target ? Target->GetCombatTargetCombatant() : nullptr;
}

void PublishElementStateChanged(UReEchoCombatantComponent& Combatant)
{
	AActor* Owner = Combatant.GetOwner();
	UReEchoCombatEventsComponent* Events =
	    Owner ? Owner->FindComponentByClass<UReEchoCombatEventsComponent>() : nullptr;
	if (!Events)
	{
		return;
	}
	FReEchoElementStateChangedEvent Event;
	Event.Combatant = &Combatant;
	Event.State = Combatant.GetElementState();
	Events->PublishElementStateChanged(Event);
}

void PublishElementReactionResolved(UReEchoCombatantComponent& Combatant,
	                                 const FReEchoElementReactionResolvedEvent& Event)
{
	if (AActor* Owner = Combatant.GetOwner())
	{
		if (UReEchoCombatEventsComponent* Events = Owner->FindComponentByClass<UReEchoCombatEventsComponent>())
		{
			Events->PublishElementReactionResolved(Event);
		}
	}
}

float GetFinalReactionMultiplier(const FReEchoElementHitResult& Result)
{
	return Result.bAppliedEnhancement ? FMath::Max(1.0f, Result.EnhancementMultiplier) : 1.0f;
}

float GetEchoMultiplier(const FReEchoElementHitResult& Result, const FReEchoElementHitContext& Context)
{
	return Result.bAffectedByEchoEfficiency ? FMath::Max(0.0f, Context.SourceEchoEfficiency) : 1.0f;
}

void AddAffected(FReEchoElementExecutionResult& Result, AActor& Target)
{
	Result.AffectedTargets.AddUnique(&Target);
}

FReEchoHitResolved
ApplyDamage(AActor& Target, const float Damage, const EReEchoElement Element, const FReEchoElementHitContext& Context)
{
	FReEchoHitIntent Intent;
	Intent.Attack = Context.Attack;
	Intent.Target = &Target;
	Intent.RawDamage = Damage;
	Intent.DamageSource = EReEchoDamageSource::Reaction;
	Intent.Element = Element;
	Intent.ReactionEfficiency = Context.ReactionEfficiency;
	Intent.bCritical = Context.bCritical;
	Intent.bSourceRulesApplied = Context.bSourceRulesApplied;
	Intent.SourceLocation = Context.SourceLocation;
	Intent.HitLocation = Target.GetActorLocation();
	return ReEchoHitResolver::ResolvePhysicalHit(Intent);
}

void ApplyStatus(FReEchoElementState& State,
                 const FName StatusId,
                 const float OverrideDurationSeconds,
                 const FReEchoElementRuleSet& Rules,
                 const float CurrentTimeSeconds)
{
	if (StatusId.IsNone() || CurrentTimeSeconds < 0.0f)
	{
		return;
	}
	const float Duration = OverrideDurationSeconds > 0.0f ? OverrideDurationSeconds : Rules.GetStatusDuration(StatusId);
	if (Duration > 0.0f)
	{
		State.ActiveStatusUntilSeconds.Add(StatusId, CurrentTimeSeconds + Duration);
	}
}

void ApplyElementalImmunity(FReEchoElementState& State,
                            const FReEchoElementRuleSet& Rules,
                            const float CurrentTimeSeconds)
{
	const float Duration = Rules.GetStatusDuration(FName(ElementImmunityStatusId));
	if (Duration > 0.0f && CurrentTimeSeconds >= 0.0f)
	{
		State.ImmunityUntil = CurrentTimeSeconds + Duration;
		State.ActiveStatusUntilSeconds.Add(FName(ElementImmunityStatusId), State.ImmunityUntil);
	}
}

bool SortTargetStable(const AActor& Left, const AActor& Right, const FVector& Origin)
{
	const float LeftDistance = FVector::DistSquared2D(Left.GetActorLocation(), Origin);
	const float RightDistance = FVector::DistSquared2D(Right.GetActorLocation(), Origin);
	if (!FMath::IsNearlyEqual(LeftDistance, RightDistance))
	{
		return LeftDistance < RightDistance;
	}
	const IReEchoCombatTarget* LeftTarget = Cast<IReEchoCombatTarget>(&Left);
	const IReEchoCombatTarget* RightTarget = Cast<IReEchoCombatTarget>(&Right);
	const int32 LeftIndex = LeftTarget ? LeftTarget->GetCombatTargetTieBreakIndex() : MAX_int32;
	const int32 RightIndex = RightTarget ? RightTarget->GetCombatTargetTieBreakIndex() : MAX_int32;
	return LeftIndex != RightIndex ? LeftIndex < RightIndex : Left.GetFName().LexicalLess(Right.GetFName());
}

TArray<AActor*>
GetAliveTargetsInRadius(UWorld& World, const FVector& Origin, const float RadiusCm, const FReEchoAttackIdentity& Attack)
{
	TArray<AActor*> Targets;
	const float RadiusSquared = FMath::Square(FMath::Max(0.0f, RadiusCm));
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Candidate = *It;
		IReEchoCombatTarget* Target = Candidate ? Cast<IReEchoCombatTarget>(Candidate) : nullptr;
		if (Target && Target->IsCombatTargetAlive() && ReEchoCombatRelations::CanDamage(Attack, *Candidate) &&
		    FVector::DistSquared2D(Target->GetCombatTargetLocation(), Origin) <= RadiusSquared)
		{
			Targets.Add(Candidate);
		}
	}
	Targets.Sort(
	    [&Origin](const AActor& Left, const AActor& Right)
	    {
		    return SortTargetStable(Left, Right, Origin);
	    });
	return Targets;
}

void ClearBurn(FReEchoElementState& State, const bool bRemoveStatus)
{
	State.bBurnActive = false;
	State.BurnTickDamage = 0.0f;
	State.BurnNextTickTimeSeconds = 0.0f;
	State.BurnSourceLocation = FVector::ZeroVector;
	State.BurnAttack = {};
	if (bRemoveStatus)
	{
		State.ActiveStatusUntilSeconds.Remove(FName(BurnStatusId));
	}
}

int32 CountRemainingBurnTicks(const float NextTick, const float EndTime)
{
	if (NextTick < 0.0f || EndTime < 0.0f || NextTick > EndTime + KINDA_SMALL_NUMBER)
	{
		return 0;
	}
	return FMath::Max(0, FMath::FloorToInt((EndTime - NextTick) / BurnTickIntervalSeconds) + 1);
}
} // namespace

int32 ReEchoHitResolver::TickElementStatuses(AActor& Target, const float CurrentTimeSeconds)
{
	UReEchoCombatantComponent* Combatant = GetCombatant(Target);
	if (!Combatant || !Combatant->IsAlive() || CurrentTimeSeconds < 0.0f)
	{
		return 0;
	}
	FReEchoElementState& State = FReEchoElementResolverAccess::Edit(*Combatant);
	float* BurnUntil = State.ActiveStatusUntilSeconds.Find(FName(BurnStatusId));
	if (!State.bBurnActive || !BurnUntil || State.BurnTickDamage <= 0.0f || State.BurnNextTickTimeSeconds < 0.0f)
	{
		ClearBurn(State, BurnUntil && CurrentTimeSeconds > *BurnUntil + KINDA_SMALL_NUMBER);
		return 0;
	}
	int32 AppliedTicks = 0;
	while (Combatant->IsAlive() && State.BurnNextTickTimeSeconds <= CurrentTimeSeconds + KINDA_SMALL_NUMBER &&
	       State.BurnNextTickTimeSeconds <= *BurnUntil + KINDA_SMALL_NUMBER)
	{
		FReEchoElementHitContext Context;
		Context.Attack = State.BurnAttack;
		Context.SourceLocation = State.BurnSourceLocation;
		ApplyDamage(Target, State.BurnTickDamage, EReEchoElement::Flame, Context);
		State.BurnNextTickTimeSeconds += BurnTickIntervalSeconds;
		++AppliedTicks;
	}
	if (State.BurnNextTickTimeSeconds > *BurnUntil + KINDA_SMALL_NUMBER || CurrentTimeSeconds > *BurnUntil)
	{
		ClearBurn(State, true);
	}
	PublishElementStateChanged(*Combatant);
	return AppliedTicks;
}

FReEchoHitResolved ReEchoHitResolver::ResolveHit(const FReEchoHitIntent& Intent)
{
	FReEchoHitIntent Candidate = Intent;
	if (!Candidate.bSourceRulesApplied)
	{
		if (const IReEchoCombatTarget* SourceRules = Cast<IReEchoCombatTarget>(Candidate.Attack.Source.Get()))
		{
			SourceRules->ModifyOutgoingHit(Candidate);
		}
		Candidate.bSourceRulesApplied = true;
	}
	if (Candidate.Element == EReEchoElement::None)
	{
		return ResolvePhysicalHit(Candidate);
	}
	FReEchoHitResolved Result;
	Result.Attack = Candidate.Attack;
	Result.Target = Candidate.Target;
	Result.RawDamage = Candidate.RawDamage;
	Result.DamageSource = Candidate.DamageSource;
	Result.Element = Candidate.Element;
	Result.bCritical = Candidate.bCritical;
	Result.HitLocation = Candidate.HitLocation;
	IReEchoCombatTarget* Target = Cast<IReEchoCombatTarget>(Candidate.Target);
	UReEchoCombatantComponent* Combatant = Target ? Target->GetCombatTargetCombatant() : nullptr;
	if (!Candidate.Target || !Target || !Combatant || !Target->IsCombatTargetAlive() ||
	    !ReEchoCombatRelations::CanDamage(Candidate.Attack, *Candidate.Target, Candidate.bAllowSameFactionDamage))
	{
		Result.bBlocked = true;
		return Result;
	}
	const bool bWasAlive = Combatant->IsAlive();
	FReEchoElementHitContext Context;
	Context.SourceLocation = Candidate.SourceLocation;
	Context.Attack = Candidate.Attack;
	Context.ReactionEfficiency = Candidate.ReactionEfficiency;
	Context.SourceElementalAttack = Candidate.RawDamage;
	Context.bCritical = Candidate.bCritical;
	Context.bSourceRulesApplied = true;
	Context.SourceEchoEfficiency = 1.0f;
	if (AActor* Source = Intent.Attack.Source.Get())
	{
		if (const UReEchoCombatantComponent* SourceCombatant =
		        Source->FindComponentByClass<UReEchoCombatantComponent>())
		{
			Context.SourceElementalAttack = SourceCombatant->Stats.ElementalAttack;
			Context.SourceEchoEfficiency = SourceCombatant->Stats.EchoEfficiency;
		}
	}
	Result.AppliedDamage =
	    ResolveElementHit(*Candidate.Target, Candidate.Element, Candidate.RawDamage, Context).ImmediateDamageApplied;
	Result.bBlocked = Result.AppliedDamage <= 0.0f;
	Result.bKilled = bWasAlive && !Combatant->IsAlive();
	return Result;
}

FReEchoElementExecutionResult ReEchoHitResolver::ResolveElementHit(AActor& Target,
                                                                   const EReEchoElement IncomingElement,
                                                                   const float BaseDamage,
                                                                   const FReEchoElementHitContext& Context)
{
	FReEchoElementExecutionResult Execution;
	TArray<FReEchoElementReactionLink> ReactionLinks;
	UReEchoCombatantComponent* PrimaryCombatant = GetCombatant(Target);
	const TSharedPtr<const FReEchoElementRuleSet> Rules = ReEchoElementRuntime::GetRuleSet();
	UWorld* World = Target.GetWorld();
	if (!PrimaryCombatant || !PrimaryCombatant->IsAlive() || !Rules.IsValid())
	{
		return Execution;
	}
	const float CurrentTime = World ? World->GetTimeSeconds() : -1.0f;
	FReEchoElementState& PrimaryState = FReEchoElementResolverAccess::Edit(*PrimaryCombatant);
	const EReEchoElement PreviousElement = PrimaryState.Attached;
	Execution.Primary = ReEchoElementRuntime::ResolveHit(
	    PrimaryState, IncomingElement, BaseDamage, Context.ReactionEfficiency, CurrentTime);
	const FReEchoReactionRuleDefinition* Reaction = Rules->Reactions.Find(Execution.Primary.ReactionId);
	if (!Execution.Primary.bTriggeredReaction || !Reaction)
	{
		const FReEchoHitResolved Hit = ApplyDamage(Target, Execution.Primary.Damage, IncomingElement, Context);
		Execution.ImmediateDamageApplied = Hit.AppliedDamage;
		AddAffected(Execution, Target);
		PublishElementStateChanged(*PrimaryCombatant);
		return Execution;
	}
	if (const IReEchoCombatTarget* SourceRules = Cast<IReEchoCombatTarget>(Context.Attack.Source.Get()))
	{
		SourceRules->NotifyReactionResolved(Execution.Primary.ReactionId);
	}

	const float FinalMultiplier = GetFinalReactionMultiplier(Execution.Primary);
	const float EchoMultiplier = GetEchoMultiplier(Execution.Primary, Context);
	if (Reaction->BehaviorId == TEXT("Reaction.Burn"))
	{
		const float TickDamage = FMath::Max(0.0f, Context.SourceElementalAttack) * Reaction->DamageMultiplier *
		                         FMath::Max(0.0f, Context.ReactionEfficiency) * FinalMultiplier * EchoMultiplier;
		const float Duration = Execution.Primary.StatusDurationSeconds > 0.0f
		                           ? Execution.Primary.StatusDurationSeconds
		                           : Rules->GetStatusDuration(FName(BurnStatusId));
		if (CurrentTime >= 0.0f && TickDamage > 0.0f && Duration > 0.0f)
		{
			const float EndTime = CurrentTime + Duration;
			const float ExistingEnd = PrimaryState.ActiveStatusUntilSeconds.FindRef(FName(BurnStatusId));
			if (PrimaryState.bBurnActive && PrimaryState.BurnNextTickTimeSeconds <= CurrentTime + KINDA_SMALL_NUMBER &&
			    PrimaryState.BurnNextTickTimeSeconds <= ExistingEnd + KINDA_SMALL_NUMBER)
			{
				TickElementStatuses(Target, CurrentTime);
			}
			const bool bRefreshOnly =
			    PrimaryState.bBurnActive &&
			    PrimaryState.ActiveStatusUntilSeconds.FindRef(FName(BurnStatusId)) > CurrentTime &&
			    PrimaryState.BurnNextTickTimeSeconds > CurrentTime;
			PrimaryState.ActiveStatusUntilSeconds.Add(FName(BurnStatusId), EndTime);
			PrimaryState.bBurnActive = true;
			if (!bRefreshOnly)
			{
				PrimaryState.BurnTickDamage = TickDamage;
				PrimaryState.BurnNextTickTimeSeconds = CurrentTime + BurnTickIntervalSeconds;
				PrimaryState.BurnSourceLocation = Context.SourceLocation;
				PrimaryState.BurnAttack = Context.Attack;
			}
			Execution.DotTicksScheduled = CountRemainingBurnTicks(PrimaryState.BurnNextTickTimeSeconds, EndTime);
			for (int32 Index = 0; Index < Execution.DotTicksScheduled; ++Index)
			{
				Execution.DotTickDelaySeconds.Add(PrimaryState.BurnNextTickTimeSeconds +
				                                  Index * BurnTickIntervalSeconds - CurrentTime);
			}
		}
		ApplyElementalImmunity(PrimaryState, *Rules, CurrentTime);
		AddAffected(Execution, Target);
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Vaporize"))
	{
		ApplyStatus(PrimaryState,
		            Execution.Primary.AppliedStatusId,
		            Execution.Primary.StatusDurationSeconds,
		            *Rules,
		            CurrentTime);
		const float ElementalAttack = FMath::Max(0.0f, Context.SourceElementalAttack);
		const float Damage = ElementalAttack * ElementalAttack * Reaction->DamageIncrease *
		                     FMath::Max(0.0f, Context.ReactionEfficiency) * FinalMultiplier * EchoMultiplier;
		Execution.ImmediateDamageApplied += ApplyDamage(Target, Damage, IncomingElement, Context).AppliedDamage;
		ApplyElementalImmunity(PrimaryState, *Rules, CurrentTime);
		AddAffected(Execution, Target);
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Growth") && World)
	{
		ApplyStatus(PrimaryState,
		            Execution.Primary.AppliedStatusId,
		            Execution.Primary.StatusDurationSeconds,
		            *Rules,
		            CurrentTime);
		const float Radius = Reaction->RadiusCm * FMath::Max(0.0f, Context.ReactionEfficiency);
		for (AActor* Candidate : GetAliveTargetsInRadius(*World, Target.GetActorLocation(), Radius, Context.Attack))
		{
			UReEchoCombatantComponent* Combatant = Candidate ? GetCombatant(*Candidate) : nullptr;
			if (!Combatant)
			{
				continue;
			}
			FReEchoElementState& State = FReEchoElementResolverAccess::Edit(*Combatant);
			if ((CurrentTime < 0.0f || State.ImmunityUntil <= CurrentTime) &&
			    State.BlockedAttachment != EReEchoElement::Grass)
			{
				State.Attached = EReEchoElement::Grass;
				AddAffected(Execution, *Candidate);
				if (Candidate != &Target)
				{
					PublishElementStateChanged(*Combatant);
				}
			}
		}
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Conduct") && World)
	{
		ApplyStatus(PrimaryState,
		            Execution.Primary.AppliedStatusId,
		            Execution.Primary.StatusDurationSeconds,
		            *Rules,
		            CurrentTime);
		const float Damage = (FMath::Max(0.0f, Context.SourceElementalAttack) + 2.0f) *
		                     FMath::Max(0.0f, Context.ReactionEfficiency) * Reaction->DamageIncrease * FinalMultiplier *
		                     EchoMultiplier;
		TSet<AActor*> Visited;
		TArray<AActor*> Queue{&Target};
		Visited.Add(&Target);
		while (!Queue.IsEmpty())
		{
			AActor* Current = Queue[0];
			Queue.RemoveAt(0, 1, EAllowShrinking::No);
			UReEchoCombatantComponent* Combatant = Current ? GetCombatant(*Current) : nullptr;
			if (!Combatant)
			{
				continue;
			}
			FReEchoElementState& State = FReEchoElementResolverAccess::Edit(*Combatant);
			State.Attached = EReEchoElement::None;
			Execution.ImmediateDamageApplied += ApplyDamage(*Current, Damage, IncomingElement, Context).AppliedDamage;
			ApplyElementalImmunity(State, *Rules, CurrentTime);
			AddAffected(Execution, *Current);
			if (Current != &Target)
			{
				PublishElementStateChanged(*Combatant);
			}
			for (AActor* Neighbor :
			     GetAliveTargetsInRadius(*World, Current->GetActorLocation(), Reaction->RadiusCm, Context.Attack))
			{
				if (!Neighbor || Visited.Contains(Neighbor))
				{
					continue;
				}
				UReEchoCombatantComponent* NeighborCombatant = GetCombatant(*Neighbor);
				if (Neighbor != &Target &&
				    (!NeighborCombatant || NeighborCombatant->GetElementState().Attached != EReEchoElement::Water))
				{
					continue;
				}
				Visited.Add(Neighbor);
				Queue.Add(Neighbor);
				FReEchoElementReactionLink& Link = ReactionLinks.AddDefaulted_GetRef();
				Link.SourceTarget = Current;
				Link.TargetTarget = Neighbor;
			}
		}
	}
	else if (Reaction->BehaviorId == TEXT("Reaction.Enhance"))
	{
		ApplyStatus(PrimaryState,
		            Execution.Primary.AppliedStatusId,
		            Execution.Primary.StatusDurationSeconds,
		            *Rules,
		            CurrentTime);
		AddAffected(Execution, Target);
	}
	PublishElementStateChanged(*PrimaryCombatant);
	FReEchoElementReactionResolvedEvent ReactionEvent;
	ReactionEvent.Attack = Context.Attack;
	ReactionEvent.ReactionId = Execution.Primary.ReactionId;
	ReactionEvent.ReactionBehaviorId = Execution.Primary.ReactionBehaviorId;
	ReactionEvent.RadiusCm = Reaction->BehaviorId == TEXT("Reaction.Growth")
	                             ? Reaction->RadiusCm * FMath::Max(0.0f, Context.ReactionEfficiency)
	                             : Execution.Primary.RadiusCm;
	ReactionEvent.PreviousElement = PreviousElement;
	ReactionEvent.IncomingElement = IncomingElement;
	ReactionEvent.ResultingElement = PrimaryState.Attached;
	ReactionEvent.PrimaryTarget = &Target;
	for (const TWeakObjectPtr<AActor>& AffectedTarget : Execution.AffectedTargets)
	{
		if (AActor* Actor = AffectedTarget.Get())
		{
			ReactionEvent.AffectedTargets.Add(Actor);
		}
	}
	ReactionEvent.ReactionLinks = MoveTemp(ReactionLinks);
	PublishElementReactionResolved(*PrimaryCombatant, ReactionEvent);
	return Execution;
}
