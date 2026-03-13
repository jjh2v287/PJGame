// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PJFeatureToggle.generated.h"

/**
 * 기능별 on/off 토글.
 * 에디터: Project Settings → Game → PJ Feature Toggles 에서 편집 가능.
 * 또는 DefaultGame.ini에서 직접 설정.
 *
 * 사용법:
 *   if (!GetDefault<UPJFeatureToggleSettings>()->bEnableCombat) return;
 *
 * 개발/프로토타입 단계에서 불필요한 시스템을 끄고 테스트할 때 사용.
 */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="PJ Feature Toggles"))
class PJGAME_API UPJFeatureToggleSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

	UPROPERTY(Config, EditAnywhere, Category = "Combat")
	bool bEnableCombat = true;

	UPROPERTY(Config, EditAnywhere, Category = "Inventory")
	bool bEnableInventory = true;

	UPROPERTY(Config, EditAnywhere, Category = "AI")
	bool bEnableMonsterAI = true;

	UPROPERTY(Config, EditAnywhere, Category = "Economy")
	bool bEnableEconomy = false;

	UPROPERTY(Config, EditAnywhere, Category = "Progression")
	bool bEnableSaveSystem = false;
};
