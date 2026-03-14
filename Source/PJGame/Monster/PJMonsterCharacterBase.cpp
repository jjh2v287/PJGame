// Copyright PJGame. All Rights Reserved.

#include "Monster/PJMonsterCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Core/PJDamageTypes.h"
#include "Core/PJGameplayTags.h"
#include "Core/PJMessageTypes.h"
#include "Core/PJStatsComponent.h"
#include "Core/PJTeamComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"

APJMonsterCharacterBase::APJMonsterCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	StatsComponent = CreateDefaultSubobject<UPJStatsComponent>(TEXT("StatsComponent"));
	TeamComponent = CreateDefaultSubobject<UPJTeamComponent>(TEXT("TeamComponent"));
	TeamComponent->SetTeamId(EPJTeamId::Monster);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));

	GetCharacterMovement()->MaxWalkSpeed = 350.f;
}

void APJMonsterCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (StatsComponent)
	{
		StatsComponent->OnDeath.AddUObject(this, &ThisClass::HandleDeath);
	}
}

FGenericTeamId APJMonsterCharacterBase::GetGenericTeamId() const
{
	return TeamComponent ? TeamComponent->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

float APJMonsterCharacterBase::ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator)
{
	if (!StatsComponent)
	{
		return 0.f;
	}

	FPJDamageSpec DamageSpec;
	DamageSpec.Amount = Amount;
	DamageSpec.DamageType = DamageType;
	DamageSpec.Instigator = DamageInstigator;
	DamageSpec.Causer = DamageInstigator;
	return StatsComponent->ApplyDamage(DamageSpec);
}

float APJMonsterCharacterBase::GetCurrentHealth_Implementation() const
{
	return StatsComponent ? StatsComponent->GetCurrentHealth() : 0.f;
}

bool APJMonsterCharacterBase::IsAlive_Implementation() const
{
	return StatsComponent && StatsComponent->IsAlive();
}

void APJMonsterCharacterBase::SetCurrentTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
}

void APJMonsterCharacterBase::HandleDeath(AActor* OwningActor, AActor* KillerActor)
{
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
	SetLifeSpan(5.f);

	if (UWorld* World = GetWorld())
	{
		FPJActorDeathMessage DeathMessage;
		DeathMessage.DeadActor = this;
		DeathMessage.Killer = KillerActor;
		DeathMessage.DeathLocation = GetActorLocation();

		UGameplayMessageSubsystem::Get(World).BroadcastMessage(PJGameplayTags::TAG_Event_Actor_Death, DeathMessage);
	}
}
