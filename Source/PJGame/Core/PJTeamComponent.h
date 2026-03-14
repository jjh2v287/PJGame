// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GenericTeamAgentInterface.h"
#include "PJTeamTypes.h"
#include "PJTeamComponent.generated.h"

UCLASS(ClassGroup=(PJGame), meta=(BlueprintSpawnableComponent))
class PJGAME_API UPJTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPJTeamComponent();

	UFUNCTION(BlueprintPure, Category = "Team")
	EPJTeamId GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeamId(EPJTeamId NewTeamId);

	FGenericTeamId GetGenericTeamId() const;

	UFUNCTION(BlueprintPure, Category = "Team")
	bool IsHostileToActor(const AActor* OtherActor) const;

	static bool AreTeamsAllied(EPJTeamId TeamA, EPJTeamId TeamB);
	static bool AreTeamsHostile(EPJTeamId TeamA, EPJTeamId TeamB);
	static const UPJTeamComponent* FindTeamComponent(const AActor* Actor);
	static UPJTeamComponent* FindTeamComponent(AActor* Actor);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Team", meta = (AllowPrivateAccess = "true"))
	EPJTeamId TeamId = EPJTeamId::Neutral;
};
