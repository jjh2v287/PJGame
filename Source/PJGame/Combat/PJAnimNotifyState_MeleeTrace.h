// Copyright PJGame. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "PJAnimNotifyState_MeleeTrace.generated.h"


/**
 * 공격 판정용 런타임 데이터 (인스턴스별).
 * 같은 몽타주를 여러 캐릭터가 동시에 재생해도 각각 독립 동작.
 */
USTRUCT()
struct FPJMeleeTraceInstanceData
{
	GENERATED_BODY()

	FVector PrevBoneWorldPos2D = FVector::ZeroVector;
	TSet<TWeakObjectPtr<AActor>> AlreadyHitActors;
	float PrevMontagePosition = 0.f;
	bool bInitialized = false;
};

/**
 * AnimNotifyState — 근접 무기 궤적 트레이스.
 *
 * 손목/무기 본의 월드 위치를 2D 평면에 투영하여 SweepTrace.
 * 프레임 드랍 시 애니메이션 로우데이터에서 중간 본 위치를 보간하여
 * 곡선 궤적의 빈틈을 메운다.
 *
 * 사용법:
 *   몽타주 타임라인에서 공격 판정 구간에 이 NotifyState를 배치.
 *   TraceBoneName, TraceBoxHalfExtent, DamageAmount 등을 에디터에서 설정.
 */
UCLASS(Blueprintable, meta = (DisplayName = "PJ Melee Weapon Trace"))
class PJGAME_API UPJAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// ── 트레이스 설정 ──

	/** 추적할 본 이름 (검끝의 단 또는 손목 등 피벗 본) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	FName TraceBoneName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool TraceIgnoreX = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool TraceIgnoreY = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	bool TraceIgnoreZ = false;

	/** 박스 트레이스 절반 크기 (X=전방, Y=좌우, Z=높이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	FVector TraceBoxHalfExtent = FVector(15.f, 30.f, 15.f);

	/**
	 * 트레이스 대상 오브젝트 타입.
	 * Pawn뿐 아니라 WorldDynamic(부서지는 상자 등)도 포함 가능.
	 * 비어있으면 Pawn + WorldDynamic을 기본으로 사용.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

	// ── 프레임 드랍 보정 (Sub-Step) ──

	/** 목표 서브스텝 간격 (1/60초 = 0.0167) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SubStep")
	float SubStepTargetDelta = 0.0167f;

	/** 최대 서브스텝 수 (성능 제한) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SubStep")
	int32 MaxSubSteps = 4;

	// ── 데미지 ──

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageAmount = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	// ── 디버그 (UE5 내장 드로우) ──

	/** None=끄기, ForOneFrame=1프레임, ForDuration=지속, Persistent=영구 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	TEnumAsByte<EDrawDebugTrace::Type> DebugDrawType = EDrawDebugTrace::None;

	// ── AnimNotifyState Overrides ──

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	/** 인스턴스별 런타임 데이터 (MeshComp 키, raw pointer — WeakPtr 해시 불안정 방지) */
	TMap<USkeletalMeshComponent*, FPJMeleeTraceInstanceData> InstanceDataMap;

	/** 현재 프레임의 본 월드 위치 (TracePlane 에 따라 투영) */
	FVector GetCurrentBoneWorldPos2D(USkeletalMeshComponent* MeshComp) const;

	/**
	 * 애니메이션 로우데이터에서 특정 시간의 본 월드 위치를 추출.
	 * 프레임 드랍 시 중간 위치 복원에 사용.
	 */
	FVector EvaluateBoneWorldPos2D(USkeletalMeshComponent* MeshComp, float AnimTime) const;

	/** SweepTrace + 히트 처리 */
	void SweepAndDetect(USkeletalMeshComponent* MeshComp, const FVector& Start, const FVector& End, FPJMeleeTraceInstanceData& Data);

	/**
	 * TracePlane 설정에 따라 3D 좌표의 특정 축을 고정하여 2D 투영.
	 *   XY → Z 를 Owner ActorLocation.Z 로 고정
	 *   XZ → Y 를 Owner ActorLocation.Y 로 고정
	 *   YZ → X 를 Owner ActorLocation.X 로 고정
	 */
	FVector ProjectToPlane(const FVector& WorldPos, const USkeletalMeshComponent* MeshComp) const;
};
