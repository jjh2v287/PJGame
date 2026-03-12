// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PJTopDownPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class PJGAME_API APJTopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APJTopDownPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SetDestinationAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TopDown")
	float ShortPressThreshold = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TopDown")
	TEnumAsByte<ECollisionChannel> CursorTraceChannel = ECC_Visibility;

private:
	void Move(const FInputActionValue& Value);
	void OnSetDestinationStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	bool TryUpdateCursorDestination(FVector& OutDestination) const;

	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;
	bool bHasValidDestination = false;
};
