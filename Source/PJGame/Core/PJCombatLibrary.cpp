// Copyright PJGame. All Rights Reserved.

#include "Core/PJCombatLibrary.h"

#include "Core/PJCoreInterfaces.h"
#include "Core/PJMessageTypes.h"
#include "Core/PJGameplayTags.h"
#include "Core/PJStatsComponent.h"
#include "Core/PJTeamComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"

float UPJCombatLibrary::TryApplyDamage(AActor* TargetActor, const FPJDamageSpec& DamageSpec)
{
	if (!TargetActor || DamageSpec.Amount <= 0.f)
	{
		return 0.f;
	}

	if (DamageSpec.Instigator.Get() == TargetActor)
	{
		return 0.f;
	}

	if (DamageSpec.Instigator && !AreActorsHostile(DamageSpec.Instigator.Get(), TargetActor))
	{
		return 0.f;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UPJDamageable::StaticClass()))
	{
		return 0.f;
	}

	const float AppliedDamage = IPJDamageable::Execute_ApplyDamage(TargetActor, DamageSpec.Amount, DamageSpec.DamageType, DamageSpec.Instigator.Get());
	if (AppliedDamage <= 0.f)
	{
		return 0.f;
	}

	if (UWorld* World = TargetActor->GetWorld())
	{
		FPJDamageAppliedMessage DamageMessage;
		DamageMessage.Instigator = DamageSpec.Instigator.Get();
		DamageMessage.Target = TargetActor;
		DamageMessage.DamageAmount = AppliedDamage;
		DamageMessage.DamageType = DamageSpec.DamageType;

		UGameplayMessageSubsystem::Get(World).BroadcastMessage(PJGameplayTags::TAG_Event_Damage_Applied, DamageMessage);
	}

	return AppliedDamage;
}

bool UPJCombatLibrary::AreActorsHostile(AActor* SourceActor, AActor* TargetActor)
{
	if (!SourceActor || !TargetActor || SourceActor == TargetActor)
	{
		return false;
	}

	return UPJTeamComponent::AreTeamsHostile(GetActorTeamId(SourceActor), GetActorTeamId(TargetActor));
}

EPJTeamId UPJCombatLibrary::GetActorTeamId(AActor* Actor)
{
	if (!Actor)
	{
		return EPJTeamId::Neutral;
	}

	if (const UPJTeamComponent* TeamComponent = Actor->FindComponentByClass<UPJTeamComponent>())
	{
		return TeamComponent->GetTeamId();
	}

	return EPJTeamId::Neutral;
}

UPJStatsComponent* UPJCombatLibrary::GetStatsComponent(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UPJStatsComponent>() : nullptr;
}

UPJTeamComponent* UPJCombatLibrary::GetTeamComponent(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UPJTeamComponent>() : nullptr;
}
