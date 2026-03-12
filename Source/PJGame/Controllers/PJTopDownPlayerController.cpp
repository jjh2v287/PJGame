// Copyright Epic Games, Inc. All Rights Reserved.

#include "PJTopDownPlayerController.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Math/RotationMatrix.h"

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

	if (SetDestinationAction)
	{
		EnhancedInputComponent->BindAction(SetDestinationAction, ETriggerEvent::Started, this, &ThisClass::OnSetDestinationStarted);
		EnhancedInputComponent->BindAction(SetDestinationAction, ETriggerEvent::Triggered, this, &ThisClass::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationAction, ETriggerEvent::Completed, this, &ThisClass::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationAction, ETriggerEvent::Canceled, this, &ThisClass::OnSetDestinationReleased);
	}
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

	const FRotator CurrentControlRotation = GetControlRotation();
	const FRotator YawRotation(0.f, CurrentControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
	ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
}

void APJTopDownPlayerController::OnSetDestinationStarted()
{
	FollowTime = 0.f;
	StopMovement();
	bHasValidDestination = TryUpdateCursorDestination(CachedDestination);
}

void APJTopDownPlayerController::OnSetDestinationTriggered()
{
	if (const UWorld* World = GetWorld())
	{
		FollowTime += World->GetDeltaSeconds();
	}

	FVector NewDestination = FVector::ZeroVector;
	if (TryUpdateCursorDestination(NewDestination))
	{
		CachedDestination = NewDestination;
		bHasValidDestination = true;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !bHasValidDestination)
	{
		return;
	}

	// Holding the input continuously steers the pawn toward the cursor.
	const FVector Direction = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
	ControlledPawn->AddMovementInput(Direction, 1.f, false);
}

void APJTopDownPlayerController::OnSetDestinationReleased()
{
	if (FollowTime <= ShortPressThreshold && bHasValidDestination)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
	}

	FollowTime = 0.f;
}

bool APJTopDownPlayerController::TryUpdateCursorDestination(FVector& OutDestination) const
{
	FHitResult HitResult;
	if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(CursorTraceChannel), true, HitResult))
	{
		return false;
	}

	OutDestination = HitResult.ImpactPoint;
	return true;
}
