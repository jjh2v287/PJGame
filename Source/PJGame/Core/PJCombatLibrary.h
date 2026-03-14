// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/PJDamageTypes.h"
#include "Core/PJTeamTypes.h"
#include "PJCombatLibrary.generated.h"

class UPJStatsComponent;
class UPJTeamComponent;

UCLASS()
class PJGAME_API UPJCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat")
	static float TryApplyDamage(AActor* TargetActor, const FPJDamageSpec& DamageSpec);

	UFUNCTION(BlueprintPure, Category = "Combat")
	static bool AreActorsHostile(AActor* SourceActor, AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Combat")
	static EPJTeamId GetActorTeamId(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Combat")
	static UPJStatsComponent* GetStatsComponent(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "Combat")
	static UPJTeamComponent* GetTeamComponent(AActor* Actor);
};
