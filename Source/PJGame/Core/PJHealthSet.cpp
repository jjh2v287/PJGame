// Copyright PJGame. All Rights Reserved.

#include "PJHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UPJHealthSet::UPJHealthSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitIncomingDamage(0.f);
	InitIncomingHeal(0.f);
}

void UPJHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Health는 [0, MaxHealth] 범위로 클램핑
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	// MaxHealth는 최소 1 이상
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
}

void UPJHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Instigator 추출
	AActor* Instigator = nullptr;
	if (Data.EffectSpec.GetContext().GetEffectCauser())
	{
		Instigator = Data.EffectSpec.GetContext().GetEffectCauser();
	}

	AActor* OwningActor = GetOwningActor();

	// ── IncomingDamage 메타 어트리뷰트 처리 ──
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageAmount = GetIncomingDamage();
		SetIncomingDamage(0.f); // 메타 어트리뷰트는 사용 후 리셋

		if (DamageAmount > 0.f)
		{
			const float OldHealth = GetHealth();
			const float NewHealth = FMath::Clamp(OldHealth - DamageAmount, 0.f, GetMaxHealth());
			SetHealth(NewHealth);

			// 변경 알림
			OnHealthChanged.Broadcast(OwningActor, OldHealth, NewHealth, Instigator);

			// 사망 판정
			if (NewHealth <= 0.f)
			{
				OnHealthDepleted.Broadcast(OwningActor);
			}
		}
	}
	// ── IncomingHeal 메타 어트리뷰트 처리 ──
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float HealAmount = GetIncomingHeal();
		SetIncomingHeal(0.f);

		if (HealAmount > 0.f)
		{
			const float OldHealth = GetHealth();
			const float NewHealth = FMath::Clamp(OldHealth + HealAmount, 0.f, GetMaxHealth());
			SetHealth(NewHealth);

			OnHealthChanged.Broadcast(OwningActor, OldHealth, NewHealth, Instigator);
		}
	}
}

void UPJHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPJHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPJHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UPJHealthSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPJHealthSet, Health, OldHealth);
}

void UPJHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPJHealthSet, MaxHealth, OldMaxHealth);
}
