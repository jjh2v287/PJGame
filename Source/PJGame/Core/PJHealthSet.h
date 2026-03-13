// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PJHealthSet.generated.h"

// GAS AttributeSet 매크로 헬퍼
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * PJHealthSet — GAS 기반 HP 어트리뷰트.
 * 
 * Health, MaxHealth를 FGameplayAttributeData로 관리.
 * GameplayEffect(GE)를 통해 데미지/힐을 적용하면
 * PreAttributeChange / PostGameplayEffectExecute에서
 * 클램핑, 사망 판정 등을 처리할 수 있다.
 */
UCLASS()
class PJGAME_API UPJHealthSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPJHealthSet();

	// ── Attributes ──

	/** 현재 체력 */
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPJHealthSet, Health)

	/** 최대 체력 */
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPJHealthSet, MaxHealth)

	/**
	 * 인커밍 데미지 — 메타 어트리뷰트.
	 * GE의 Modifier가 이 값을 설정하면,
	 * PostGameplayEffectExecute에서 Health에서 차감하고 0으로 리셋.
	 * 직접 Replicate하지 않음.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UPJHealthSet, IncomingDamage)

	/**
	 * 인커밍 힐 — 메타 어트리뷰트.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UPJHealthSet, IncomingHeal)

	// ── Delegates ──

	/** Health가 0에 도달했을 때 발화 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthDepleted, AActor* /*OwningActor*/);
	FOnHealthDepleted OnHealthDepleted;

	/** Health 값이 변경되었을 때 발화 */
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnHealthChanged, AActor* /*OwningActor*/, float /*OldValue*/, float /*NewValue*/, AActor* /*Instigator*/);
	FOnHealthChanged OnHealthChanged;

protected:
	// ── UAttributeSet Overrides ──

	/** 값이 실제로 변경되기 전에 호출 — 클램핑 용도 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** GE 실행 후 호출 — 메타 어트리뷰트 처리 (IncomingDamage → Health 차감) */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
};
