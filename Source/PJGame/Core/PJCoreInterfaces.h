// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "PJCoreInterfaces.generated.h"

// ─────────────────────────────────────────────
// IDamageable — 데미지를 받을 수 있는 액터
// ─────────────────────────────────────────────

UINTERFACE(MinimalAPI, Blueprintable)
class UPJDamageable : public UInterface
{
	GENERATED_BODY()
};

class PJGAME_API IPJDamageable
{
	GENERATED_BODY()

public:
	/**
	 * 데미지를 적용한다.
	 * @param Amount      데미지 양
	 * @param DamageType  데미지 유형 태그 (e.g. Damage.Physical, Damage.Fire)
	 * @param Instigator  데미지를 가한 액터
	 * @return 실제 적용된 데미지 양
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
	float ApplyDamage(float Amount, FGameplayTag DamageType, AActor* DamageInstigator);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
	bool IsAlive() const;
};

// ─────────────────────────────────────────────
// IInteractable — 상호작용 가능한 액터
// ─────────────────────────────────────────────

UINTERFACE(MinimalAPI, Blueprintable)
class UPJInteractable : public UInterface
{
	GENERATED_BODY()
};

class PJGAME_API IPJInteractable
{
	GENERATED_BODY()

public:
	/** 상호작용을 실행한다 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Instigator);

	/** UI에 표시할 상호작용 설명 텍스트 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;

	/** 현재 상호작용 가능한 상태인지 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Instigator) const;
};
