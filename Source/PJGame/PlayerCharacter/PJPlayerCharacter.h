// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "PJCoreInterfaces.h"
#include "PJPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UPJHealthSet;

/**
 * 탑다운 플레이어 캐릭터.
 * 
 * - IAbilitySystemInterface: GAS와 연동
 * - IPJDamageable: Core 인터페이스로 데미지 수신
 * 
 * ASC와 HealthSet을 소유하며, IPJDamageable 구현을 통해
 * 어떤 시스템이든 Cast<IPJDamageable>로 데미지를 적용할 수 있다.
 */
UCLASS()
class PJGAME_API APJPlayerCharacter : public ACharacter, public IAbilitySystemInterface, public IPJDamageable
{
	GENERATED_BODY()

public:
	APJPlayerCharacter();

	// ── IAbilitySystemInterface ──
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ── IPJDamageable ──
	virtual float ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;

protected:
	virtual void BeginPlay() override;

	// ── Camera ──
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

	// ── GAS ──
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPJHealthSet> HealthSet;

	/** HealthSet의 사망 델리게이트 콜백 */
	void HandleHealthDepleted(AActor* OwningActor);
};
