// Copyright PJGame. All Rights Reserved.

#include "PJAnimNotifyState_MeleeTrace.h"

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

	// ── 서브스텝 수 결정 ──
	int32 NumSubSteps = FMath::CeilToInt(FrameDeltaTime / SubStepTargetDelta);
	NumSubSteps = FMath::Clamp(NumSubSteps, 1, MaxSubSteps);

	if (NumSubSteps <= 1)
	{
		// 프레임 드랍 없음: 단순 Sweep
		SweepAndDetect(MeshComp, Data.PrevBoneWorldPos2D, CurrentPos, Data);
	}
	else
	{
		// ── 프레임 드랍 감지 → 애니메이션 로우데이터 보간 ──
		UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
		UAnimMontage* Montage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;

		if (Montage)
		{
			const float CurrentMontagePos = AnimInstance->Montage_GetPosition(Montage);
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

			Data.PrevMontagePosition = CurrentMontagePos;
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
	}

	Data.PrevBoneWorldPos2D = CurrentPos;
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
// GetCurrentBoneWorldPos2D — 현재 프레임의 본 위치 (2D)
// ─────────────────────────────────────────────

FVector UPJAnimNotifyState_MeleeTrace::GetCurrentBoneWorldPos2D(USkeletalMeshComponent* MeshComp) const
{
	FVector Pos = MeshComp->GetSocketLocation(TraceBoneName);

	// Z를 캐릭터 높이로 고정 → 탑다운 2D 평면 투영
	if (AActor* Owner = MeshComp->GetOwner())
	{
		Pos.Z = Owner->GetActorLocation().Z;
	}

	return Pos;
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

	// ── 5단계: 컴포넌트 → 월드 → 2D 투영 ──
	FVector BoneWorldPos = MeshComp->GetComponentTransform().TransformPosition(BoneCS.GetLocation());

	if (AActor* Owner = MeshComp->GetOwner())
	{
		BoneWorldPos.Z = Owner->GetActorLocation().Z;
	}

	return BoneWorldPos;
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
		if (HitActor->GetClass()->ImplementsInterface(UPJDamageable::StaticClass()))
		{
			IPJDamageable::Execute_ApplyDamage(HitActor, DamageAmount, DamageTypeTag, Owner);
		}
	}
}
