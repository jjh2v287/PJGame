// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/PJDamageTypes.h"
#include "PJStatsComponent.generated.h"

UCLASS(ClassGroup=(PJGame), meta=(BlueprintSpawnableComponent))
class PJGAME_API UPJStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPJStatsComponent();

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, AActor* /*OwningActor*/, float /*OldHealth*/, float /*NewHealth*/);
	DECLARE_MULTICAST_DELEGATE_FourParams(FOnDamageReceived, AActor* /*OwningActor*/, AActor* /*InstigatorActor*/, float /*AppliedDamage*/, FGameplayTag /*DamageType*/);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDeath, AActor* /*OwningActor*/, AActor* /*InstigatorActor*/);

	FOnHealthChanged OnHealthChanged;
	FOnDamageReceived OnDamageReceived;
	FOnDeath OnDeath;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float ApplyDamage(const FPJDamageSpec& DamageSpec);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float ApplyHeal(float Amount, AActor* HealInstigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RestoreToMaxHealth();

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttackPower() const { return AttackPower; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsAlive() const { return !bIsDead && CurrentHealth > 0.f; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPower = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	bool bStartAtMaxHealth = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;
};
