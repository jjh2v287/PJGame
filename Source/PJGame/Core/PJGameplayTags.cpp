// Copyright PJGame. All Rights Reserved.

#include "PJGameplayTags.h"

namespace PJGameplayTags
{
	// ── 이벤트 태그 ──
	UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Damage_Applied,  "Event.Damage.Applied");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Actor_Death,     "Event.Actor.Death");

	// ── 데미지 유형 ──
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Physical, "Damage.Physical");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Fire,     "Damage.Fire");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Ice,      "Damage.Ice");

	// ── 캐릭터 상태 ──
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dead,    "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Rolling, "State.Rolling");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stunned, "State.Stunned");

	// ── 입력 태그 ──
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Move,     "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Attack,   "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Roll,     "Input.Roll");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Jump,     "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Interact, "Input.Interact");
}
