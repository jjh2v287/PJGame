// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controllers/PJTopDownPlayerController.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Core/PJCombatLibrary.h"
#include "Core/PJCoreInterfaces.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "PlayerCharacter/PJPlayerCharacter.h"

APJTopDownPlayerController::APJTopDownPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void APJTopDownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!DefaultMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void APJTopDownPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("APJTopDownPlayerController requires an EnhancedInputComponent."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	}

	if (MouseLeftDownAction)
	{
		EnhancedInputComponent->BindAction(MouseLeftDownAction, ETriggerEvent::Triggered, this, &ThisClass::OnMouseLeftDown);
	}

	if (RollAction)
	{
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &ThisClass::StartRoll);
	}
	
	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::StartJump);
	}
}

void APJTopDownPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		FHitResult HitResult;
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(CursorTraceChannel), true, HitResult))
		{
			FVector TargetLocation = HitResult.ImpactPoint;
			FVector PawnLocation = ControlledPawn->GetActorLocation();
			TargetLocation.Z = PawnLocation.Z;

			const FRotator LookAtRotation = (TargetLocation - PawnLocation).Rotation();
			ControlledPawn->SetActorRotation(LookAtRotation);
		}
	}

	UpdateQueuedAttack();
}

void APJTopDownPlayerController::Move(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsNearlyZero())
	{
		return;
	}

	ClearQueuedAttack();
	StopMovement();

	const FVector ForwardDirection = FVector::ForwardVector;
	const FVector RightDirection = FVector::RightVector;

	ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
	ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
}

void APJTopDownPlayerController::OnMouseLeftDown()
{
	FHitResult HitResult;
	if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(CursorTraceChannel), true, HitResult))
	{
		HandleLeftClickHit(HitResult);
	}
}

void APJTopDownPlayerController::StartRoll()
{
	if (!GetPawn())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Player Rolled!"));
}

void APJTopDownPlayerController::StartJump()
{
	if (ACharacter* PlayerCharacterPawn = Cast<ACharacter>(GetPawn()))
	{
		PlayerCharacterPawn->Jump();
	}
}

void APJTopDownPlayerController::HandleLeftClickHit(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	APJPlayerCharacter* PlayerCharacter = GetPJPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	if (HitActor &&
		HitActor->GetClass()->ImplementsInterface(UPJDamageable::StaticClass()) &&
		UPJCombatLibrary::AreActorsHostile(PlayerCharacter, HitActor))
	{
		IssueAttackCommand(HitActor);
		return;
	}

	IssueMoveCommand(HitResult.ImpactPoint);
}

void APJTopDownPlayerController::IssueMoveCommand(const FVector& WorldLocation)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	ClearQueuedAttack();
	StopMovement();

	FVector MoveDirection = WorldLocation - ControlledPawn->GetActorLocation();
	MoveDirection.Z = 0.f;

	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	ControlledPawn->AddMovementInput(MoveDirection.GetSafeNormal(), 1.0f);
}

void APJTopDownPlayerController::IssueAttackCommand(AActor* TargetActor)
{
	APJPlayerCharacter* PlayerCharacter = GetPJPlayerCharacter();
	if (!PlayerCharacter || !TargetActor)
	{
		return;
	}

	QueuedAttackTarget = TargetActor;
	bAttackMoveQueued = true;

	const float DistanceToTarget = FVector::Dist2D(PlayerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
	const float DesiredRange = PlayerCharacter->GetDesiredAttackRange() + PlayerCharacter->GetAttackAcceptanceRadius();

	if (DistanceToTarget > DesiredRange)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToActor(this, TargetActor);
		return;
	}

	StopMovement();
	if (PlayerCharacter->ExecutePrimaryAttack(TargetActor))
	{
		ClearQueuedAttack();
	}
}

void APJTopDownPlayerController::UpdateQueuedAttack()
{
	if (!bAttackMoveQueued)
	{
		return;
	}

	APJPlayerCharacter* PlayerCharacter = GetPJPlayerCharacter();
	AActor* TargetActor = QueuedAttackTarget.Get();
	if (!PlayerCharacter || !TargetActor)
	{
		ClearQueuedAttack();
		return;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UPJDamageable::StaticClass()) ||
		!IPJDamageable::Execute_IsAlive(TargetActor) ||
		!UPJCombatLibrary::AreActorsHostile(PlayerCharacter, TargetActor))
	{
		ClearQueuedAttack();
		return;
	}

	const float AttackDistance = FVector::Dist2D(PlayerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
	const float DesiredRange = PlayerCharacter->GetDesiredAttackRange() + PlayerCharacter->GetAttackAcceptanceRadius();

	if (AttackDistance > DesiredRange)
	{
		return;
	}

	StopMovement();
	if (PlayerCharacter->ExecutePrimaryAttack(TargetActor))
	{
		ClearQueuedAttack();
	}
}

void APJTopDownPlayerController::ClearQueuedAttack()
{
	QueuedAttackTarget.Reset();
	bAttackMoveQueued = false;
}

APJPlayerCharacter* APJTopDownPlayerController::GetPJPlayerCharacter() const
{
	return Cast<APJPlayerCharacter>(GetPawn());
}
