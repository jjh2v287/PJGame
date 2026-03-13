// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PJCoreTypes.generated.h"

/**
 * 아이템 스택 — 아이템 태그 + 수량의 단순 묶음.
 * 인벤토리, 드랍, 거래 등 모든 아이템 이동에서 공용으로 사용.
 */
USTRUCT(BlueprintType)
struct PJGAME_API FPJItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 1;

	FPJItemStack() = default;
	FPJItemStack(FGameplayTag InTag, int32 InQuantity)
		: ItemTag(InTag), Quantity(InQuantity) {}
};
