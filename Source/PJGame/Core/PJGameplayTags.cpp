// Copyright PJGame. All Rights Reserved.

#include "Core/PJGameplayTags.h"

namespace PJGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Damage_Applied, "Event.Damage.Applied");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Actor_Death, "Event.Actor.Death");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Caravan_Destroyed, "Event.Caravan.Destroyed");

	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Physical, "Damage.Physical");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Fire, "Damage.Fire");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Ice, "Damage.Ice");

	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dead, "State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Rolling, "State.Rolling");
	UE_DEFINE_GAMEPLAY_TAG(TAG_State_Stunned, "State.Stunned");

	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Move, "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Attack, "Input.Attack");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Roll, "Input.Roll");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Jump, "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Interact, "Input.Interact");
}
