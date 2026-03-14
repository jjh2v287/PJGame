// Copyright PJGame. All Rights Reserved.

#include "Core/PJStatsComponent.h"

#include "GameFramework/Actor.h"

UPJStatsComponent::UPJStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPJStatsComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(MaxHealth, 1.f);
	CurrentHealth = bStartAtMaxHealth ? MaxHealth : FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
	bIsDead = CurrentHealth <= 0.f;
}

float UPJStatsComponent::ApplyDamage(const FPJDamageSpec& DamageSpec)
{
	if (bIsDead || DamageSpec.Amount <= 0.f)
	{
		return 0.f;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageSpec.Amount, 0.f, MaxHealth);

	const float AppliedDamage = OldHealth - CurrentHealth;
	if (AppliedDamage <= 0.f)
	{
		return 0.f;
	}

	AActor* OwnerActor = GetOwner();
	OnHealthChanged.Broadcast(OwnerActor, OldHealth, CurrentHealth);
	OnDamageReceived.Broadcast(OwnerActor, DamageSpec.Instigator.Get(), AppliedDamage, DamageSpec.DamageType);

	if (CurrentHealth <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast(OwnerActor, DamageSpec.Instigator.Get());
	}

	return AppliedDamage;
}

float UPJStatsComponent::ApplyHeal(const float Amount, AActor* HealInstigator)
{
	if (bIsDead || Amount <= 0.f)
	{
		return 0.f;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);

	const float AppliedHeal = CurrentHealth - OldHealth;
	if (AppliedHeal > 0.f)
	{
		OnHealthChanged.Broadcast(GetOwner(), OldHealth, CurrentHealth);
	}

	return AppliedHeal;
}

void UPJStatsComponent::RestoreToMaxHealth()
{
	if (bIsDead)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(GetOwner(), OldHealth, CurrentHealth);
}
