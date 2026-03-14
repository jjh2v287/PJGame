// Copyright PJGame. All Rights Reserved.

#include "Caravan/PJCaravanActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/PJDamageTypes.h"
#include "Core/PJGameplayTags.h"
#include "Core/PJMessageTypes.h"
#include "Core/PJStatsComponent.h"
#include "Core/PJTeamComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/GameplayMessageSubsystem.h"

APJCaravanActor::APJCaravanActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CaravanMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CaravanMesh"));
	CaravanMesh->SetupAttachment(Root);
	CaravanMesh->SetCollisionProfileName(TEXT("Caravan"));

	StatsComponent = CreateDefaultSubobject<UPJStatsComponent>(TEXT("StatsComponent"));
	TeamComponent = CreateDefaultSubobject<UPJTeamComponent>(TEXT("TeamComponent"));
	TeamComponent->SetTeamId(EPJTeamId::Caravan);
}

void APJCaravanActor::BeginPlay()
{
	Super::BeginPlay();

	if (StatsComponent)
	{
		StatsComponent->OnDeath.AddUObject(this, &ThisClass::HandleDeath);
	}
}

void APJCaravanActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFollow(DeltaSeconds);
}

FGenericTeamId APJCaravanActor::GetGenericTeamId() const
{
	return TeamComponent ? TeamComponent->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

float APJCaravanActor::ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator)
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

float APJCaravanActor::GetCurrentHealth_Implementation() const
{
	return StatsComponent ? StatsComponent->GetCurrentHealth() : 0.f;
}

bool APJCaravanActor::IsAlive_Implementation() const
{
	return StatsComponent && StatsComponent->IsAlive();
}

void APJCaravanActor::SetFollowTarget(AActor* NewFollowTarget)
{
	FollowTarget = NewFollowTarget;
}

void APJCaravanActor::AddStoredItem(const FPJItemStack& ItemStack)
{
	if (!ItemStack.ItemTag.IsValid() || ItemStack.Quantity <= 0)
	{
		return;
	}

	StoredItems.Add(ItemStack);
}

TArray<FPJItemStack> APJCaravanActor::ExtractStoredItems()
{
	TArray<FPJItemStack> DroppedItems = StoredItems;
	StoredItems.Reset();
	return DroppedItems;
}

void APJCaravanActor::UpdateFollow(const float DeltaSeconds)
{
	if (!FollowTarget || !StatsComponent || !StatsComponent->IsAlive())
	{
		return;
	}

	const FVector DesiredLocation = FollowTarget->GetActorLocation() - FollowTarget->GetActorForwardVector() * FollowDistance;
	FVector OffsetToTarget = DesiredLocation - GetActorLocation();
	OffsetToTarget.Z = 0.f;

	if (OffsetToTarget.SizeSquared() <= FMath::Square(StopDistance))
	{
		return;
	}

	const float MoveSpeed = OffsetToTarget.Size() > CatchUpDistance ? FollowMoveSpeed * 1.5f : FollowMoveSpeed;
	const FVector NewLocation = GetActorLocation() + OffsetToTarget.GetSafeNormal() * MoveSpeed * DeltaSeconds;
	SetActorLocation(FVector(NewLocation.X, NewLocation.Y, GetActorLocation().Z));

	if (!OffsetToTarget.IsNearlyZero())
	{
		SetActorRotation(OffsetToTarget.Rotation());
	}
}

void APJCaravanActor::HandleDeath(AActor* OwningActor, AActor* KillerActor)
{
	SetActorEnableCollision(false);

	const TArray<FPJItemStack> DroppedItems = ExtractStoredItems();

	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);

		FPJActorDeathMessage DeathMessage;
		DeathMessage.DeadActor = this;
		DeathMessage.Killer = KillerActor;
		DeathMessage.DeathLocation = GetActorLocation();
		MessageSubsystem.BroadcastMessage(PJGameplayTags::TAG_Event_Actor_Death, DeathMessage);

		FPJCaravanDestroyedMessage CaravanDestroyedMessage;
		CaravanDestroyedMessage.CaravanActor = this;
		CaravanDestroyedMessage.Killer = KillerActor;
		CaravanDestroyedMessage.DestroyedLocation = GetActorLocation();
		CaravanDestroyedMessage.DroppedItems = DroppedItems;
		MessageSubsystem.BroadcastMessage(PJGameplayTags::TAG_Event_Caravan_Destroyed, CaravanDestroyedMessage);
	}
}
