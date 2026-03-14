// Copyright PJGame. All Rights Reserved.

#include "PJAnimNotifyState_MeleeTrace.h"

#include "Core/PJCombatLibrary.h"
#include "Core/PJDamageTypes.h"
#include "PJCoreInterfaces.h"
#include "PJGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

namespace
{
void DrawTraceDebugSphere(
	const UWorld* World,
	const EDrawDebugTrace::Type DrawType,
	const FVector& Position,
	const float Radius,
	const FColor& Color)
{
	if (!World || DrawType == EDrawDebugTrace::None)
	{
		return;
	}

	const bool bPersistent = DrawType == EDrawDebugTrace::Persistent;
	const float LifeTime = DrawType == EDrawDebugTrace::ForDuration ? 1.0f : 0.0f;

	DrawDebugSphere(World, Position, Radius, 12, Color, bPersistent, LifeTime, 0, 1.5f);
}
}

// ─────────────────────────────────────────────
// NotifyBegin — 판정 시작, 인스턴스 데이터 초기화
// ─────────────────────────────────────────────

void UPJAnimNotifyState_MeleeTrace::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	FPJMeleeTraceInstanceData& Data = InstanceDataMap.FindOrAdd(MeshComp);
	Data.AlreadyHitActors.Reset();
	Data.PrevBoneWorldPos2D = GetCurrentBoneWorldPos2D(MeshComp);
	Data.bInitialized = true;

	// 현재 몽타주 위치 캐싱
	if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
	{
		if (UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage())
		{
			Data.PrevMontagePosition = AnimInstance->Montage_GetPosition(Montage);
		}
	}
}

// ─────────────────────────────────────────────
// NotifyTick — 매 프레임 궤적 추적 + 서브스텝
// ─────────────────────────────────────────────

void UPJAnimNotifyState_MeleeTrace::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp) return;

	FPJMeleeTraceInstanceData* DataPtr = InstanceDataMap.Find(MeshComp);
	if (!DataPtr || !DataPtr->bInitialized) return;

	FPJMeleeTraceInstanceData& Data = *DataPtr;

	// 현재 본 위치 (2D 투영)
	const FVector CurrentPos = GetCurrentBoneWorldPos2D(MeshComp);

	// ── 현재 몽타주 위치 (분기 전에 미리 조회 — PrevMontagePosition 항상 갱신을 위해) ──
	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;
	const float CurrentMontagePos = Montage ? AnimInstance->Montage_GetPosition(Montage) : 0.f;

	DrawTraceDebugSphere(MeshComp->GetWorld(), DebugDrawType, Data.PrevBoneWorldPos2D, 10, FColor::Red);
	DrawTraceDebugSphere(MeshComp->GetWorld(), DebugDrawType, CurrentPos, 15, FColor::Green);

	// ── 서브스텝 수 결정 ──
	int32 NumSubSteps = FMath::CeilToInt(FrameDeltaTime / SubStepTargetDelta);
	NumSubSteps = FMath::Clamp(NumSubSteps, 1, MaxSubSteps);

	if (NumSubSteps <= 1)
	{
		// 프레임 드랍 없음: 단순 Sweep
		SweepAndDetect(MeshComp, Data.PrevBoneWorldPos2D, CurrentPos, Data);
	}
	else if (Montage)
	{
		const float PrevMontagePos = Data.PrevMontagePosition;
		FVector PrevPos = Data.PrevBoneWorldPos2D;

		for (int32 i = 1; i <= NumSubSteps; ++i)
		{
			const float Alpha = static_cast<float>(i) / NumSubSteps;

			FVector SamplePos;
			if (i == NumSubSteps)
			{
				// 마지막 스텝은 현재 프레임의 실제 본 위치 사용 (정확도)
				SamplePos = CurrentPos;
			}
			else
			{
				// 중간 스텝: 몽타주 시간을 보간하여 애니메이션에서 본 위치 복원
				const float SampleMontageTime = FMath::Lerp(PrevMontagePos, CurrentMontagePos, Alpha);
				SamplePos = EvaluateBoneWorldPos2D(MeshComp, SampleMontageTime);
			}

			SweepAndDetect(MeshComp, PrevPos, SamplePos, Data);
			PrevPos = SamplePos;
		}
	}
	else
	{
		// 몽타주가 아닌 경우: 선형 보간 폴백
		FVector PrevPos = Data.PrevBoneWorldPos2D;
		for (int32 i = 1; i <= NumSubSteps; ++i)
		{
			const float Alpha = static_cast<float>(i) / NumSubSteps;
			const FVector SamplePos = FMath::Lerp(Data.PrevBoneWorldPos2D, CurrentPos, Alpha);
			SweepAndDetect(MeshComp, PrevPos, SamplePos, Data);
			PrevPos = SamplePos;
		}
	}

	// ── 항상 갱신 (분기에 관계없이) ──
	Data.PrevBoneWorldPos2D = CurrentPos;
	if (Montage)
	{
		Data.PrevMontagePosition = CurrentMontagePos;
	}
}

// ─────────────────────────────────────────────
// NotifyEnd — 판정 종료, 정리
// ─────────────────────────────────────────────

void UPJAnimNotifyState_MeleeTrace::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp)
	{
		InstanceDataMap.Remove(MeshComp);
	}
}

FString UPJAnimNotifyState_MeleeTrace::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("MeleeTrace [%s]"), *TraceBoneName.ToString());
}

// ─────────────────────────────────────────────
// GetCurrentBoneWorldPos2D — 현재 프레임의 본 위치 (TracePlane 투영)
// ─────────────────────────────────────────────

FVector UPJAnimNotifyState_MeleeTrace::GetCurrentBoneWorldPos2D(USkeletalMeshComponent* MeshComp) const
{
	const FVector Pos = MeshComp->GetSocketLocation(TraceBoneName);
	return ProjectToPlane(Pos, MeshComp);
}

// ─────────────────────────────────────────────
// EvaluateBoneWorldPos2D — 애니메이션 로우데이터에서 특정 시간의 본 위치 복원
// ─────────────────────────────────────────────

FVector UPJAnimNotifyState_MeleeTrace::EvaluateBoneWorldPos2D(
	USkeletalMeshComponent* MeshComp,
	float MontageTime) const
{
	// 폴백: 현재 프레임 본 위치
	const FVector FallbackPos = GetCurrentBoneWorldPos2D(MeshComp);

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance) return FallbackPos;

	UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();
	if (!Montage) return FallbackPos;

	// ── 1단계: 몽타주에서 해당 시간의 AnimSequence 추출 ──
	const UAnimSequence* AnimSeq = nullptr;
	float SequenceTime = 0.f;

	for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
	{
		for (const FAnimSegment& Segment : SlotTrack.AnimTrack.AnimSegments)
		{
			const float SegEnd = Segment.StartPos + Segment.GetLength();
			if (MontageTime >= Segment.StartPos && MontageTime <= SegEnd)
			{
				const float TimeIntoSegment = MontageTime - Segment.StartPos;
				SequenceTime = Segment.AnimStartTime + TimeIntoSegment * Segment.AnimPlayRate;
				AnimSeq = Cast<UAnimSequence>(Segment.GetAnimReference().Get());
				break;
			}
		}
		if (AnimSeq) break;
	}

	if (!AnimSeq) return FallbackPos;

	// ── 2단계: 본 인덱스 확인 ──
	const int32 BoneIndex = MeshComp->GetBoneIndex(TraceBoneName);
	if (BoneIndex == INDEX_NONE) return FallbackPos;

	const FBoneContainer& RequiredBones = AnimInstance->GetRequiredBones();
	if (!RequiredBones.IsValid()) return FallbackPos;

	const FCompactPoseBoneIndex CompactBoneIndex = RequiredBones.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneIndex));
	if (CompactBoneIndex == INDEX_NONE) return FallbackPos;

	// ── 3단계: 애니메이션 로우데이터에서 포즈 추출 ──
	FCompactPose CompactPose;
	CompactPose.SetBoneContainer(&RequiredBones);
	CompactPose.ResetToRefPose();

	FBlendedCurve Curves;
	Curves.InitFrom(RequiredBones);

	UE::Anim::FStackAttributeContainer Attributes;
	FAnimationPoseData PoseData(CompactPose, Curves, Attributes);

	FAnimExtractContext ExtractionCtx(static_cast<double>(SequenceTime), false);
	AnimSeq->GetAnimationPose(PoseData, ExtractionCtx);

	// ── 4단계: 로컬 → 컴포넌트 스페이스 변환 ──
	FCSPose<FCompactPose> CSPose;
	CSPose.InitPose(CompactPose);
	const FTransform BoneCS = CSPose.GetComponentSpaceTransform(CompactBoneIndex);

	// ── 5단계: 컴포넌트 → 월드 → TracePlane 투영 ──
	const FVector BoneWorldPos = MeshComp->GetComponentTransform().TransformPosition(BoneCS.GetLocation());
	return ProjectToPlane(BoneWorldPos, MeshComp);
}

// ─────────────────────────────────────────────
// SweepAndDetect — BoxTrace + 히트 처리
// ─────────────────────────────────────────────

void UPJAnimNotifyState_MeleeTrace::SweepAndDetect(
	USkeletalMeshComponent* MeshComp,
	const FVector& Start,
	const FVector& End,
	FPJMeleeTraceInstanceData& Data)
{
	if (Start.Equals(End, 1.f)) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return;

	// ── 오브젝트 타입 결정 ──
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = TraceObjectTypes;
	if (ObjectTypes.IsEmpty())
	{
		// 기본값: Pawn + WorldDynamic (부서지는 상자 등)
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Destructible));
	}

	// ── 자기 자신 무시 ──
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);

	// ── 박스 방향: Start→End 방향으로 회전 ──
	const FRotator TraceRotation = (End - Start).Rotation();

	// ── BoxTraceMultiForObjects (UE5 내장 디버그 포함) ──
	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::BoxTraceMultiForObjects(
		Owner,
		Start,
		End,
		TraceBoxHalfExtent,
		TraceRotation,
		ObjectTypes,
		false,               // bTraceComplex
		ActorsToIgnore,
		DebugDrawType,        // UE5 내장 디버그 드로잉
		HitResults,
		true                  // bIgnoreSelf
	);

	// ── 히트 처리 ──
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == Owner) continue;

		// 중복 히트 방지
		TWeakObjectPtr<AActor> WeakHit(HitActor);
		if (Data.AlreadyHitActors.Contains(WeakHit)) continue;
		Data.AlreadyHitActors.Add(WeakHit);

		// IPJDamageable 인터페이스로 데미지 적용
		FPJDamageSpec DamageSpec;
		DamageSpec.Amount = DamageAmount;
		DamageSpec.DamageType = DamageTypeTag;
		DamageSpec.Instigator = Owner;
		DamageSpec.Causer = Owner;
		UPJCombatLibrary::TryApplyDamage(HitActor, DamageSpec);
	}
}

// ─────────────────────────────────────────────
// ProjectToPlane — TracePlane 에 따라 특정 축을 Actor 위치로 고정
// ─────────────────────────────────────────────

FVector UPJAnimNotifyState_MeleeTrace::ProjectToPlane(
	const FVector& WorldPos,
	const USkeletalMeshComponent* MeshComp) const
{
	if (!MeshComp) return WorldPos;

	const AActor* Owner = MeshComp->GetOwner();
	if (!Owner) return WorldPos;

	// 메시 로컬 축 기준으로 평면을 고정해야 메시 상대 회전(-90 yaw 등)이 있어도
	// 인게임과 프리뷰에서 동일한 축 기준으로 투영된다.
	const FTransform& MeshTransform = MeshComp->GetComponentTransform();
	const FVector LocalPos = MeshTransform.InverseTransformPosition(WorldPos);
	const FVector LocalOrigin = MeshTransform.InverseTransformPosition(Owner->GetActorLocation());

	FVector ProjectedLocal = LocalPos;
	
	if (TraceIgnoreX)
	{
		ProjectedLocal.X = LocalOrigin.X;
	}
	
	if (TraceIgnoreY)
	{
		ProjectedLocal.Y = LocalOrigin.Y;
	}
	
	if (TraceIgnoreZ)
	{
		ProjectedLocal.Z = LocalOrigin.Z;
	}

	return MeshTransform.TransformPosition(ProjectedLocal);
}
