// Copyright Epic Games, Inc. All Rights Reserved.

#include "PJTopDownPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
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

	if (ClickAction)
	{
		// 마우스 클릭 시(혹은 설정된 키가 눌렸을 때) 한 번 호출됩니다.
		// 누르고 있을 때 계속 호출되기를 원하신다면 ETriggerEvent::Triggered 로 변경하시면 됩니다.
		EnhancedInputComponent->BindAction(ClickAction, ETriggerEvent::Started, this, &ThisClass::OnClickStarted);
	}
}

void APJTopDownPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		FHitResult HitResult;
		// 마우스 커서 위치로 레이캐스트를 쏴서 월드 상의 위치를 구함
		if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(CursorTraceChannel), true, HitResult))
		{
			FVector TargetLocation = HitResult.ImpactPoint;
			FVector PawnLocation = ControlledPawn->GetActorLocation();
			
			// 캐릭터가 위아래로 기울어지지 않도록 Z축(높이)은 캐릭터의 위치와 동일하게 맞춤
			TargetLocation.Z = PawnLocation.Z;

			// 캐릭터가 마우스 커서 위치를 바라보도록 회전값 계산 후 적용
			FRotator LookAtRotation = (TargetLocation - PawnLocation).Rotation();
			ControlledPawn->SetActorRotation(LookAtRotation);
		}
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

	// 마우스 클릭 이동 중 WASD 입력 시 기존 이동 취소
	StopMovement();

	// 탑다운 뷰에서는 카메라 방향이 고정되어 있으므로 절대 월드 축을 기준으로 이동합니다.
	// W,S는 월드의 X축, A,D는 월드의 Y축으로 이동하게 됩니다.
	const FVector ForwardDirection = FVector::ForwardVector;
	const FVector RightDirection = FVector::RightVector;

	ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
	ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
}

void APJTopDownPlayerController::OnClickStarted()
{
	FHitResult HitResult;
	// 마우스 커서 위치로 레이캐스트를 쏴서 월드 상의 타격점을 구함
	if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(CursorTraceChannel), true, HitResult))
	{
		// 클릭한 위치로 네비게이션 시스템을 이용해 캐릭터 이동
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitResult.ImpactPoint);
	}
}
