// Copyright PJGame. All Rights Reserved.

#include "PJPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "PJHealthSet.h"
#include "PJGameplayTags.h"
#include "PJMessageTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/SpringArmComponent.h"

APJPlayerCharacter::APJPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->RotationRate = FRotator(0.f, 720.f, 0.f);
	MovementComponent->MaxWalkSpeed = 600.f;
	MovementComponent->bConstrainToPlane = false;
	MovementComponent->SetPlaneConstraintNormal(FVector::UpVector);
	MovementComponent->bSnapToPlaneAtStart = false;

	// ── Camera ──
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bInheritYaw = false;

	// 부드러운 카메라 추적(Camera Lag) 활성화
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 3.f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// ── GAS ──
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// HealthSet은 ASC의 서브오브젝트로 생성 — ASC가 자동으로 등록 관리
	HealthSet = CreateDefaultSubobject<UPJHealthSet>(TEXT("HealthSet"));
}

void APJPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// ASC 초기화
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// HealthSet 사망 델리게이트 바인딩
	if (HealthSet)
	{
		HealthSet->OnHealthDepleted.AddUObject(this, &ThisClass::HandleHealthDepleted);
	}
}

// ─────────────────────────────────────────────
// IAbilitySystemInterface
// ─────────────────────────────────────────────

UAbilitySystemComponent* APJPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ─────────────────────────────────────────────
// IPJDamageable — 어떤 시스템이든 이 인터페이스로 데미지 적용 가능
// ─────────────────────────────────────────────

float APJPlayerCharacter::ApplyDamage_Implementation(float Amount, FGameplayTag DamageType, AActor* DamageInstigator)
{
	if (!AbilitySystemComponent || Amount <= 0.f)
	{
		return 0.f;
	}

	// GE를 동적으로 생성하여 IncomingDamage 메타 어트리뷰트에 데미지 적용
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		UGameplayEffect::StaticClass(), 1.f, AbilitySystemComponent->MakeEffectContext());

	if (SpecHandle.IsValid())
	{
		// IncomingDamage에 Additive Modifier로 데미지 값 설정
		SpecHandle.Data->SetSetByCallerMagnitude(DamageType, Amount);
	}

	// 간단한 직접 적용 방식 (SetByCallerMagnitude 대신)
	// PostGameplayEffectExecute에서 처리되도록 직접 어트리뷰트 수정
	const float OldHealth = HealthSet->GetHealth();
	const float NewHealth = FMath::Clamp(OldHealth - Amount, 0.f, HealthSet->GetMaxHealth());
	AbilitySystemComponent->SetNumericAttributeBase(UPJHealthSet::GetHealthAttribute(), NewHealth);

	// 변경 알림 (UI 등이 구독)
	HealthSet->OnHealthChanged.Broadcast(this, OldHealth, NewHealth, DamageInstigator);

	// 사망 판정
	if (NewHealth <= 0.f)
	{
		HealthSet->OnHealthDepleted.Broadcast(this);
	}

	return OldHealth - NewHealth;
}

float APJPlayerCharacter::GetCurrentHealth_Implementation() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.f;
}

bool APJPlayerCharacter::IsAlive_Implementation() const
{
	return HealthSet && HealthSet->GetHealth() > 0.f;
}

// ─────────────────────────────────────────────
// Death
// ─────────────────────────────────────────────

void APJPlayerCharacter::HandleHealthDepleted(AActor* OwningActor)
{
	UE_LOG(LogTemp, Warning, TEXT("%s has died!"), *GetName());

	// 이벤트 버스로 사망 메시지 발행 — 다른 시스템이 반응 가능
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& MsgSys = UGameplayMessageSubsystem::Get(World);

		FPJActorDeathMessage DeathMsg;
		DeathMsg.DeadActor = this;
		DeathMsg.DeathLocation = GetActorLocation();

		MsgSys.BroadcastMessage(PJGameplayTags::TAG_Event_Actor_Death, DeathMsg);
	}
}
