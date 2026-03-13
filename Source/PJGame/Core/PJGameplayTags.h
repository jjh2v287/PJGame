// Copyright PJGame. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * 프로젝트 전체에서 사용하는 GameplayTag 상수 정의.
 * 
 * 태그 추가 방법:
 *   1. 이 헤더에 UE_DECLARE_GAMEPLAY_TAG_EXTERN 선언
 *   2. .cpp에 UE_DEFINE_GAMEPLAY_TAG 정의
 *   3. 코드에서 PJGameplayTags::TAG_xxx 로 참조
 *
 * 참고: .ini에서도 태그를 추가할 수 있지만,
 *       C++에서 상수로 자주 참조하는 태그는 여기에 정의하는 것이 안전합니다.
 */
namespace PJGameplayTags
{
	// ── 이벤트 태그 (GameplayMessage 채널) ──
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Damage_Applied);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Actor_Death);

	// ── 데미지 유형 ──
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Physical);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Fire);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Ice);

	// ── 캐릭터 상태 ──
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Rolling);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stunned);

	// ── 입력 태그 (Enhanced Input + GAS 연동 시 활용) ──
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Roll);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Interact);
}
