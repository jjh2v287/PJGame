// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PJCoreTypes.h"
#include "GameFramework/Actor.h"
#include "PJMessageTypes.generated.h"

/**
 * 시스템 간 통신용 메시지 구조체.
 * 
 * 사용법:
 *   UGameplayMessageSubsystem& MsgSys = UGameplayMessageSubsystem::Get(GetWorld());
 *   MsgSys.BroadcastMessage(TAG, Message);
 *
 * 규칙:
 *   - 이 파일은 Core 폴더에 위치하여 모든 시스템이 참조 가능
 *   - 새 이벤트가 필요하면 여기에 구조체를 추가
 *   - 시스템 간 직접 #include 대신 이 메시지로 통신
 */

// ─────────────────────────────────────────────
// 데미지 관련
// ─────────────────────────────────────────────

/** 누군가에게 데미지가 적용되었을 때 */
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

/** 액터가 사망했을 때 */
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
