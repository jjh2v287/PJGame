// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PJTeamTypes.generated.h"

UENUM(BlueprintType)
enum class EPJTeamId : uint8
{
	Neutral     UMETA(DisplayName = "Neutral"),
	Player      UMETA(DisplayName = "Player"),
	Caravan     UMETA(DisplayName = "Caravan"),
	Monster     UMETA(DisplayName = "Monster"),
	FriendlyNPC UMETA(DisplayName = "Friendly NPC"),
};
