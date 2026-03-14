// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "Core/PJCoreInterfaces.h"
#include "Core/PJCoreTypes.h"
#include "PJCaravanActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UPJStatsComponent;
class UPJTeamComponent;

UCLASS()
class PJGAME_API APJCaravanActor : public AActor, public IGenericTeamAgentInterface, public IPJDamageable
{
	GENERATED_BODY()

public:
	APJCaravanActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual float ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator) override;
	virtual float GetCurrentHealth_Implementation() const override;
	virtual bool IsAlive_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Caravan")
	void SetFollowTarget(AActor* NewFollowTarget);

	UFUNCTION(BlueprintCallable, Category = "Caravan")
	void AddStoredItem(const FPJItemStack& ItemStack);

	UFUNCTION(BlueprintCallable, Category = "Caravan")
	TArray<FPJItemStack> ExtractStoredItems();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CaravanMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPJStatsComponent> StatsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPJTeamComponent> TeamComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow")
	float FollowDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow")
	float StopDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow")
	float CatchUpDistance = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Follow")
	float FollowMoveSpeed = 350.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Follow")
	TObjectPtr<AActor> FollowTarget;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<FPJItemStack> StoredItems;

private:
	void UpdateFollow(const float DeltaSeconds);
	void HandleDeath(AActor* OwningActor, AActor* KillerActor);
};
