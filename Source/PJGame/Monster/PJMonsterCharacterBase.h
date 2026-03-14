// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Core/PJCoreInterfaces.h"
#include "PJMonsterCharacterBase.generated.h"

class UPJStatsComponent;
class UPJTeamComponent;

UCLASS()
class PJGAME_API APJMonsterCharacterBase : public ACharacter, public IGenericTeamAgentInterface, public IPJDamageable
{
	GENERATED_BODY()

public:
	APJMonsterCharacterBase();

	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual float ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetCurrentTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Combat")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPJStatsComponent> StatsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPJTeamComponent> TeamComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float ContactDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	bool bPrioritizeCaravan = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<AActor> CurrentTarget;

private:
	void HandleDeath(AActor* OwningActor, AActor* KillerActor);
};
