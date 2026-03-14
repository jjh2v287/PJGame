// Copyright PJGame. All Rights Reserved.

#include "PlayerCharacter/PJPlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/PJCombatLibrary.h"
#include "Core/PJDamageTypes.h"
#include "Core/PJGameplayTags.h"
#include "Core/PJMessageTypes.h"
#include "Core/PJStatsComponent.h"
#include "Core/PJTeamComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/SpringArmComponent.h"

APJPlayerCharacter::APJPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->RotationRate = FRotator(0.f, 720.f, 0.f);
	MovementComponent->MaxWalkSpeed = 600.f;
	MovementComponent->bConstrainToPlane = false;
	MovementComponent->SetPlaneConstraintNormal(FVector::UpVector);
	MovementComponent->bSnapToPlaneAtStart = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 900.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 3.f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	StatsComponent = CreateDefaultSubobject<UPJStatsComponent>(TEXT("StatsComponent"));
	TeamComponent = CreateDefaultSubobject<UPJTeamComponent>(TEXT("TeamComponent"));
	TeamComponent->SetTeamId(EPJTeamId::Player);
}

void APJPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (StatsComponent)
	{
		StatsComponent->OnDeath.AddUObject(this, &ThisClass::HandleDeath);
	}
}

FGenericTeamId APJPlayerCharacter::GetGenericTeamId() const
{
	return TeamComponent ? TeamComponent->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

float APJPlayerCharacter::ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator)
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

float APJPlayerCharacter::GetCurrentHealth_Implementation() const
{
	return StatsComponent ? StatsComponent->GetCurrentHealth() : 0.f;
}

bool APJPlayerCharacter::IsAlive_Implementation() const
{
	return StatsComponent && StatsComponent->IsAlive();
}

float APJPlayerCharacter::GetDesiredAttackRange() const
{
	return AttackMode == EPJAttackMode::Ranged ? RangedAttackRange : MeleeAttackRange;
}

bool APJPlayerCharacter::IsAttackReady() const
{
	if (!StatsComponent || !StatsComponent->IsAlive())
	{
		return false;
	}

	return GetWorld() && (GetWorld()->GetTimeSeconds() - LastAttackTime) >= AttackCooldown;
}

bool APJPlayerCharacter::ExecutePrimaryAttack(AActor* TargetActor)
{
	if (!TargetActor || !IsAttackReady())
	{
		return false;
	}

	const FVector ToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.Rotation());
	}

	if (PrimaryAttackMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_Play(PrimaryAttackMontage);
		}
	}

	if (AttackMode == EPJAttackMode::Ranged || !PrimaryAttackMontage)
	{
		FPJDamageSpec DamageSpec;
		DamageSpec.Amount = StatsComponent ? StatsComponent->GetAttackPower() : 0.f;
		DamageSpec.DamageType = PJGameplayTags::TAG_Damage_Physical;
		DamageSpec.Instigator = this;
		DamageSpec.Causer = this;
		UPJCombatLibrary::TryApplyDamage(TargetActor, DamageSpec);
	}

	LastAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAttackTime;
	return true;
}

void APJPlayerCharacter::HandleDeath(AActor* OwningActor, AActor* KillerActor)
{
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);

	if (UWorld* World = GetWorld())
	{
		FPJActorDeathMessage DeathMessage;
		DeathMessage.DeadActor = this;
		DeathMessage.Killer = KillerActor;
		DeathMessage.DeathLocation = GetActorLocation();

		UGameplayMessageSubsystem::Get(World).BroadcastMessage(PJGameplayTags::TAG_Event_Actor_Death, DeathMessage);
	}
}
