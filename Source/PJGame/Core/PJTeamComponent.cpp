// Copyright PJGame. All Rights Reserved.

#include "Core/PJTeamComponent.h"

#include "GameFramework/Actor.h"

UPJTeamComponent::UPJTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPJTeamComponent::SetTeamId(const EPJTeamId NewTeamId)
{
	TeamId = NewTeamId;
}

FGenericTeamId UPJTeamComponent::GetGenericTeamId() const
{
	return FGenericTeamId(static_cast<uint8>(TeamId));
}

bool UPJTeamComponent::IsHostileToActor(const AActor* OtherActor) const
{
	const UPJTeamComponent* OtherTeamComponent = FindTeamComponent(OtherActor);
	return AreTeamsHostile(TeamId, OtherTeamComponent ? OtherTeamComponent->GetTeamId() : EPJTeamId::Neutral);
}

bool UPJTeamComponent::AreTeamsAllied(const EPJTeamId TeamA, const EPJTeamId TeamB)
{
	if (TeamA == TeamB && TeamA != EPJTeamId::Neutral)
	{
		return true;
	}

	const auto IsTravelerTeam = [](const EPJTeamId TeamId)
	{
		return TeamId == EPJTeamId::Player || TeamId == EPJTeamId::Caravan || TeamId == EPJTeamId::FriendlyNPC;
	};

	return IsTravelerTeam(TeamA) && IsTravelerTeam(TeamB);
}

bool UPJTeamComponent::AreTeamsHostile(const EPJTeamId TeamA, const EPJTeamId TeamB)
{
	if (TeamA == EPJTeamId::Neutral || TeamB == EPJTeamId::Neutral)
	{
		return false;
	}

	return !AreTeamsAllied(TeamA, TeamB);
}

const UPJTeamComponent* UPJTeamComponent::FindTeamComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UPJTeamComponent>() : nullptr;
}

UPJTeamComponent* UPJTeamComponent::FindTeamComponent(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UPJTeamComponent>() : nullptr;
}
