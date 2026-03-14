// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Core/PJCoreInterfaces.h"
#include "PJPlayerCharacter.generated.h"

class UAnimMontage;
class UCameraComponent;
class USpringArmComponent;
class UPJStatsComponent;
class UPJTeamComponent;

UENUM(BlueprintType)
enum class EPJAttackMode : uint8
{
	Melee  UMETA(DisplayName = "Melee"),
	Ranged UMETA(DisplayName = "Ranged"),
};

UCLASS()
class PJGAME_API APJPlayerCharacter : public ACharacter, public IGenericTeamAgentInterface, public IPJDamageable
{
	GENERATED_BODY()

public:
	APJPlayerCharacter();

	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual float ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetDesiredAttackRange() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackAcceptanceRadius() const { return AttackAcceptanceRadius; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttackReady() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ExecutePrimaryAttack(AActor* TargetActor);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPJStatsComponent> StatsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPJTeamComponent> TeamComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EPJAttackMode AttackMode = EPJAttackMode::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float MeleeAttackRange = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float RangedAttackRange = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackAcceptanceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackCooldown = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> PrimaryAttackMontage;

private:
	void HandleDeath(AActor* OwningActor, AActor* KillerActor);

	float LastAttackTime = -1000.f;
};
