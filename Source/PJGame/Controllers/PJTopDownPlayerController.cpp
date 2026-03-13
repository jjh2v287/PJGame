// Copyright Epic Games, Inc. All Rights Reserved.

#include "PJTopDownPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
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

	if (MouseLeftDownAction)
	{
		// 마우스 클릭 시(혹은 설정된 키가 눌렸을 때) 한 번 호출됩니다.
		// 누르고 있을 때 계속 호출되기를 원하신다면 ETriggerEvent::Triggered 로 변경하시면 됩니다.
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

void APJTopDownPlayerController::OnMouseLeftDown()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FHitResult HitResult;
	// 마우스 커서 위치로 레이캐스트를 쏴서 월드 상의 타격점을 구함
	if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(CursorTraceChannel), true, HitResult))
	{
		FVector TargetLocation = HitResult.ImpactPoint;
		FVector PawnLocation = ControlledPawn->GetActorLocation();

		// 캐릭터가 위아래로 기울어지지 않도록 Z축(높이)은 캐릭터의 위치와 동일하게 맞춤
		TargetLocation.Z = PawnLocation.Z;

		// 캐릭터에서 마우스 커서를 향하는 방향 벡터 계산
		FVector Direction = (TargetLocation - PawnLocation).GetSafeNormal();

		// 해당 방향으로 이동 입력 추가 (네비게이션이 아닌 단순 방향 밀기)
		ControlledPawn->AddMovementInput(Direction, 1.0f);
	}
}

void APJTopDownPlayerController::StartRoll()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// 구르기 관련 로직을 여기에 작성하세요.
	// 일반적으로 다음과 같은 방식으로 처리합니다:
	// 1. 구르기 애니메이션 몽타주(AnimMontage)를 재생합니다.
	// 2. 캐릭터의 상태를 '구르기 중'으로 변경하여 이동/공격을 막습니다.
	// 3. (선택) Root Motion이나 LaunchCharacter를 통해 구르는 방향으로 힘을 가합니다.

	UE_LOG(LogTemp, Warning, TEXT("Player Rolled!"));
	
	// 예시: 캐릭터에게 밀어내는 힘을 주어 구르는 듯한 느낌을 줄 수 있습니다. (필요 시 주석 해제)
	// if (ACharacter* MyCharacter = Cast<ACharacter>(ControlledPawn))
	// {
	//     // 현재 이동 중인 방향(가속도)이나 바라보는 방향을 기준으로 설정
	//     FVector RollDirection = MyCharacter->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal();
	//     if (RollDirection.IsNearlyZero())
	//     {
	//         RollDirection = MyCharacter->GetActorForwardVector();
	//     }
	//     
	//     // Z(위) 방향으로 살짝 띄우면서 앞으로 밀어냅니다.
	//     FVector RollVelocity = RollDirection * 1000.f + FVector(0.f, 0.f, 200.f);
	//     MyCharacter->LaunchCharacter(RollVelocity, true, true);
	// }
}

void APJTopDownPlayerController::StartJump()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Player Jump!"));
}
