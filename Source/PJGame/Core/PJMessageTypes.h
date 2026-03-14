// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/PJCoreTypes.h"
#include "GameFramework/Actor.h"
#include "PJMessageTypes.generated.h"

USTRUCT(BlueprintType)
struct PJGAME_API FPJDamageAppliedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float DamageAmount = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FGameplayTag DamageType;
};

USTRUCT(BlueprintType)
struct PJGAME_API FPJActorDeathMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> DeadActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Killer = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FVector DeathLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct PJGAME_API FPJCaravanDestroyedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> CaravanActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Killer = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FVector DestroyedLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TArray<FPJItemStack> DroppedItems;
};
