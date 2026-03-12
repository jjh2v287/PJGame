# UE5 Vertical Slice 반복 이터레이션 아키텍처

**어떤 게임이든 Vertical Slice를 빠르게 만들고, 테스트하고, 부수고, 다시 만드는 사이클을 최소 비용으로 반복할 수 있는 UE5 프로젝트 시스템 구축 가이드**

---

## 1. 이 문서의 목적

1인 개발자 또는 소규모 팀이 UE5(C++ 중심)로 게임을 개발할 때, **Vertical Slice(핵심 경험의 최소 완결 버전)** 를 빠르게 반복하기 위한 프로젝트 아키텍처를 정의한다.

이 아키텍처의 핵심 목표는 다음 세 가지다:

- **수치를 바꿀 때 C++ 컴파일을 하지 않는다** (코드 vs 데이터 분리)
- **A 시스템을 부숴도 B 시스템은 돌아간다** (시스템 간 약결합)
- **기능을 Actor에 꽂았다 뺐다 할 수 있다** (상속 대신 조합)

---

## 2. 이터레이션을 죽이는 3대 원인

아키텍처를 설계하기 전에, 왜 반복이 느려지는지 먼저 진단해야 한다.

### 2.1 강결합 (Tight Coupling)

**증상**: A 시스템 수정 → B, C가 컴파일 에러.

**근본 문제**: 시스템이 서로 직접 `#include`하고 포인터를 들고 있음.

```cpp
// ❌ 강결합 — 카라반이 몬스터, 아이템, UI를 직접 참조
#include "MonsterAI.h"
#include "ItemDropManager.h"
#include "UIManager.h"

void UCaravanComponent::OnDestroyed()
{
    ItemDropManager->ScatterItems(Inventory);      // 직접 호출
    MonsterAI->NotifyItemsDropped(GetLocation());  // 직접 호출
    UIManager->ShowCaravanDestroyedAlert();         // 직접 호출
}
// → 몬스터 AI를 갈아엎으면 카라반 코드가 깨짐
```

### 2.2 하드코딩 데이터

**증상**: 수치 하나 바꾸려면 C++ 컴파일 3분 대기.

**근본 문제**: 밸런스 수치, 콘텐츠 정의가 코드 안에 박혀 있음.

### 2.3 모놀리식 구조

**증상**: 전투만 테스트하고 싶은데 인벤토리, 경제, 세이브가 전부 초기화되어야 실행됨.

**근본 문제**: 모든 기능이 한 덩어리에 엉켜 있음.

이 세 가지를 **구조적으로 불가능하게** 만드는 아키텍처가 이 문서의 핵심이다.

---

## 3. 핵심 원칙 — 3개의 분리 축

빠른 이터레이션을 위한 아키텍처의 본질은 **세 가지를 분리**하는 것이다.

```
┌─────────────────────────────────────────────┐
│  분리 축 1: 코드 vs 데이터                    │
│  "수치를 바꿀 때 컴파일하지 않는다"            │
├─────────────────────────────────────────────┤
│  분리 축 2: 시스템 vs 시스템                   │
│  "A를 부숴도 B는 돌아간다"                     │
├─────────────────────────────────────────────┤
│  분리 축 3: 기능 vs 존재                      │
│  "Actor는 껍데기, 기능은 꽂았다 뺀다"          │
└─────────────────────────────────────────────┘
```

---

## 4. 분리 축 1 — 코드 vs 데이터 (Data-Driven Layer)

### 4.1 설계 목표

C++ 컴파일 없이 게임플레이 파라미터를 조정할 수 있어야 한다.

### 4.2 구현 3종 세트

#### 4.2.1 UPrimaryDataAsset — 콘텐츠 정의의 단일 진실점

아이템, 몬스터, 스킬, NPC 등 모든 콘텐츠 정의를 DataAsset으로 관리한다.

```cpp
UCLASS()
class UItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // 분류 태그 — 시스템 간 통신의 공용 언어
    UPROPERTY(EditDefaultsOnly, Category = "Classification")
    FGameplayTagContainer ItemTags;

    // 기본 수치
    UPROPERTY(EditDefaultsOnly, Category = "Economy")
    float BaseValue = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Inventory")
    float Weight = 1.0f;

    // 지연 로드 — 메시/텍스처는 필요할 때만 메모리에 올림
    UPROPERTY(EditDefaultsOnly, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditDefaultsOnly, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;
};
```

**핵심 포인트**:

- `TSoftObjectPtr` 사용으로 아이템 정의 수백 개를 만들어도 실제 메시/텍스처는 화면에 필요할 때만 로드됨
- 프로토타입에서는 Mesh 없이 흰색 큐브만 쓰다가, DataAsset의 Mesh 필드만 교체하면 됨
- 새 아이템 추가 시 C++ 컴파일 없이 에디터에서 DataAsset만 생성

#### 4.2.2 UDataTable — 밸런스 시트

전투 데미지 계수, 몬스터 스폰 확률, 가격 테이블 등 수치 데이터를 외부에서 관리한다.

```cpp
// 테이블 행 구조체 정의 (C++ 한 번만)
USTRUCT(BlueprintType)
struct FMonsterSpawnRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FGameplayTag MonsterTag;
    UPROPERTY(EditAnywhere) float SpawnWeight = 1.0f;
    UPROPERTY(EditAnywhere) int32 MinLevel = 1;
    UPROPERTY(EditAnywhere) int32 MaxCount = 5;
};
```

**워크플로우**:

1. 행 구조체를 C++로 한 번 정의
2. 에디터에서 DataTable 에셋 생성
3. CSV로 Export → 스프레드시트에서 수백 개 수치 일괄 조정 → Import
4. 에디터에서 바로 플레이 테스트
5. C++ 리컴파일: 0회

#### 4.2.3 GameplayTag — 만능 분류 체계

```ini
# DefaultGameplayTags.ini (예시 태그 구조)
+GameplayTags=(TagName="Item.Type.Food")
+GameplayTags=(TagName="Item.Type.Mineral")
+GameplayTags=(TagName="Item.Type.MagicMaterial")
+GameplayTags=(TagName="Monster.Preference.Food")
+GameplayTags=(TagName="Caravan.State.Normal")
+GameplayTags=(TagName="Caravan.State.Damaged")
+GameplayTags=(TagName="Event.Caravan.Destroyed")
+GameplayTags=(TagName="Event.Item.Dropped")
+GameplayTags=(TagName="Event.Monster.ConsumedItem")
```

**태그의 역할**:

- 아이템 카테고리, 몬스터 선호도, NPC 선호 품목, 업그레이드 조건, 도감 분류가 **전부 같은 태그 체계를 공유**
- 문자열 기반이라 새 카테고리 추가 시 C++ 수정 불필요
- `.ini` 파일 또는 에디터에서 태그 추가 → 즉시 사용 가능
- 시스템 간 통신의 공용 언어

---

## 5. 분리 축 2 — 시스템 vs 시스템 (Event-Driven Decoupling)

### 5.1 설계 목표

전투 시스템을 통째로 갈아엎어도 인벤토리 시스템은 한 줄도 안 바뀐다.

### 5.2 이벤트 버스 — GameplayMessageSubsystem

UE5에 내장된 `GameplayMessageSubsystem`(Lyra에서 도입)을 활용한다. 서로 연결되지 않은 게임플레이 오브젝트들이 직접 참조 없이 통신할 수 있다.

#### 5.2.1 메시지 구조체 정의

```cpp
// MessageTypes.h — /Source/Core/ 에 위치 (모든 시스템이 참조 가능)
USTRUCT(BlueprintType)
struct FCaravanDestroyedMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FVector Location;
    UPROPERTY(BlueprintReadWrite) TArray<FItemStack> DroppedItems;
    UPROPERTY(BlueprintReadWrite) AActor* CaravanActor = nullptr;
};

USTRUCT(BlueprintType)
struct FItemDroppedMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FVector Location;
    UPROPERTY(BlueprintReadWrite) FGameplayTag ItemTag;
    UPROPERTY(BlueprintReadWrite) int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FMonsterConsumedItemMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) AActor* Monster = nullptr;
    UPROPERTY(BlueprintReadWrite) FGameplayTag ConsumedItemTag;
};
```

#### 5.2.2 약결합 통신 패턴

**발행자 (Publisher) — 카라반 시스템:**

```cpp
// CaravanComponent.cpp — 다른 시스템을 #include하지 않음
#include "GameFramework/GameplayMessageSubsystem.h"
#include "MessageTypes.h"  // Core 모듈의 메시지 구조체만 참조

void UCaravanComponent::OnDestroyed()
{
    // 메시지 하나만 발행. 누가 듣든 말든 카라반은 모름.
    FCaravanDestroyedMessage Msg;
    Msg.Location = GetOwner()->GetActorLocation();
    Msg.DroppedItems = Inventory->GetAllItems();
    Msg.CaravanActor = GetOwner();

    UGameplayMessageSubsystem& MsgSys =
        UGameplayMessageSubsystem::Get(GetWorld());
    MsgSys.BroadcastMessage(TAG_Event_Caravan_Destroyed, Msg);
}
```

**구독자 (Subscriber) — 아이템 드랍 시스템:**

```cpp
// ItemDropSubsystem.cpp — 카라반을 모름, 메시지만 들음
void UItemDropSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UGameplayMessageSubsystem& MsgSys =
        UGameplayMessageSubsystem::Get(GetWorld());

    ListenerHandle = MsgSys.RegisterListener<FCaravanDestroyedMessage>(
        TAG_Event_Caravan_Destroyed,
        [this](FGameplayTag Tag, const FCaravanDestroyedMessage& Msg)
        {
            ScatterItemsAtLocation(Msg.Location, Msg.DroppedItems);
        });
}
```

**구독자 — 몬스터 인식 중계:**

```cpp
// MonsterPerceptionRelay.cpp — 카라반도, 아이템 시스템도 모름
void UMonsterPerceptionRelay::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UGameplayMessageSubsystem& MsgSys =
        UGameplayMessageSubsystem::Get(GetWorld());

    ListenerHandle = MsgSys.RegisterListener<FCaravanDestroyedMessage>(
        TAG_Event_Caravan_Destroyed,
        [this](FGameplayTag Tag, const FCaravanDestroyedMessage& Msg)
        {
            NotifyNearbyMonsters(Msg.Location, Msg.DroppedItems);
        });
}
```

**구독자 — UI 시스템:**

```cpp
// GameHUDSubsystem.cpp — 게임 로직을 모름, 메시지만 들음
ListenerHandle = MsgSys.RegisterListener<FCaravanDestroyedMessage>(
    TAG_Event_Caravan_Destroyed,
    [this](FGameplayTag Tag, const FCaravanDestroyedMessage& Msg)
    {
        ShowAlertWidget(TEXT("카라반이 파괴되었습니다!"));
    });
```

#### 5.2.3 의존성 다이어그램 비교

```
[ 강결합 — 스파게티 ]
CaravanSystem ──→ MonsterAI
       │──→ ItemDropSystem
       │──→ UISystem
       └──→ EconomySystem
MonsterAI ──→ ItemDropSystem
       └──→ UISystem

→ 하나를 고치면 연쇄 컴파일, 순환 의존 위험

[ 약결합 — 이벤트 버스 ]
CaravanSystem ──→ EventBus ←── MonsterAI
                  EventBus ←── ItemDropSystem
                  EventBus ←── UISystem
                  EventBus ←── EconomySystem

→ 시스템 간 직접 참조 없음
→ 몬스터 AI 전체를 삭제해도 카라반, 아이템 시스템은 컴파일 에러 0
→ 새 시스템 추가 = 기존 코드 수정 없이 리스너 등록만
```

### 5.3 Subsystem 아키텍처

UE5의 Subsystem은 라이프사이클이 자동 관리되는 싱글턴이다. 용도에 따라 적절한 타입을 선택한다.

| Subsystem 타입 | 수명 | 적합한 용도 |
|---|---|---|
| `UGameInstanceSubsystem` | 게임 시작 ~ 종료 (레벨 전환에도 유지) | 세이브/로드, 도감, 영구 진행도, 글로벌 설정 |
| `UWorldSubsystem` | 레벨 로드 ~ 언로드 | 아이템 드랍 관리, 경제, AI Director, 몬스터 스폰, 오브젝트 풀 |
| `ULocalPlayerSubsystem` | 로컬 플레이어 존재 동안 | 플레이어 입력 설정, UI 상태, 카메라 프리셋 |

```cpp
// 예: 세이브/로드는 레벨 전환에도 유지되어야 함
UCLASS()
class USaveLoadSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    void SaveGame(const FString& SlotName);

    UFUNCTION(BlueprintCallable)
    void LoadGame(const FString& SlotName);
};

// 예: 몬스터 스폰은 현재 월드에 종속
UCLASS()
class UMonsterSpawnSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    // 월드가 언로드되면 자동으로 Deinitialize → 메모리 누수 방지
};
```

---

## 6. 분리 축 3 — 기능 vs 존재 (Component Injection)

### 6.1 설계 목표

Actor는 빈 껍데기로 두고, 기능은 런타임에 꽂았다 뺐다 한다.

### 6.2 상속 vs 조합

```
[ 나쁜 구조 — 깊은 상속 트리 ]

              ABaseCharacter
             /              \
  APlayerCharacter     AMonsterCharacter
       |                      |
  ACaravanCharacter    ABossMonster

→ 공통 기능 수정 시 전체 트리가 흔들림
→ "카라반인데 전투도 하는 NPC" 같은 새 개념에 대응 불가
→ 다중 상속 불가로 기능 조합의 유연성 없음


[ 좋은 구조 — 컴포넌트 조합 ]

APawn (빈 껍데기)
  ├── UHealthComponent         ← 필요한 Actor에만 부착
  ├── UInventoryComponent      ← 플레이어, 카라반, 몬스터 모두 가능
  ├── UCombatComponent         ← 전투 가능한 Actor에만
  ├── UCaravanBehaviorComp     ← 카라반에만
  └── UItemConsumptionComp     ← 몬스터에만

→ 새 Actor 타입 = 기존 컴포넌트 조합만 변경
→ 컴포넌트 하나를 리팩터해도 다른 컴포넌트는 무관
→ "전투하는 카라반" = UCombatComponent + UCaravanBehaviorComp 조합
```

### 6.3 컴포넌트 설계 원칙

```cpp
// 예: 인벤토리 컴포넌트 — 플레이어, 카라반, 보물상자 모두 부착 가능
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // 슬롯 타입
    UPROPERTY(EditAnywhere, Category = "Inventory")
    int32 MaxSlots = 20;

    UPROPERTY(EditAnywhere, Category = "Inventory")
    float MaxWeight = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Inventory")
    int32 ProtectedSlots = 0; // 파괴 시에도 보존되는 슬롯 수

    // 기능
    UFUNCTION(BlueprintCallable)
    bool TryAddItem(const UItemDefinition* ItemDef, int32 Quantity);

    UFUNCTION(BlueprintCallable)
    TArray<FItemStack> RemoveAllItems();

    UFUNCTION(BlueprintCallable)
    TArray<FItemStack> GetProtectedItems() const;

    // 이벤트 (UI나 다른 시스템이 구독)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FOnInventoryChanged, const UItemDefinition*, Item, int32, NewCount);
    UPROPERTY(BlueprintAssignable)
    FOnInventoryChanged OnInventoryChanged;

private:
    UPROPERTY()
    TArray<FItemStack> Items;
};
```

핵심은 이 컴포넌트가 **자신이 어떤 Actor에 붙어있는지 신경 쓰지 않는다**는 점이다. 플레이어든 카라반이든 보물상자든, `UInventoryComponent`만 부착하면 인벤토리 기능이 생긴다.

### 6.4 인터페이스로 기능 발견

컴포넌트 조합 구조에서는 "이 Actor가 데미지를 받을 수 있는가?"를 어떻게 아느냐가 문제다. **UInterface**를 사용한다.

```cpp
// CoreInterfaces.h — /Source/Core/ 에 위치
UINTERFACE(MinimalAPI, Blueprintable)
class UDamageable : public UInterface { GENERATED_BODY() };

class IDamageable
{
    GENERATED_BODY()
public:
    virtual float TakeDamage(float Amount, FGameplayTag DamageType) = 0;
    virtual float GetCurrentHealth() const = 0;
    virtual bool IsAlive() const = 0;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface { GENERATED_BODY() };

class IInteractable
{
    GENERATED_BODY()
public:
    virtual void Interact(AActor* Instigator) = 0;
    virtual FText GetInteractionPrompt() const = 0;
};
```

사용 측에서는 인터페이스만 확인하면 된다:

```cpp
// 전투 시스템 — 대상이 플레이어든 카라반이든 상관없음
if (IDamageable* Target = Cast<IDamageable>(HitActor))
{
    Target->TakeDamage(Damage, TAG_DamageType_Physical);
}
```

### 6.5 Feature Toggle — 1인 개발자를 위한 실용적 모듈 관리

UE5의 GameFeature Plugin은 런타임에 컴포넌트를 Actor에 주입할 수 있는 강력한 시스템이다. 그러나 1인 개발에서 풀 GameFeature 구현은 초기 셋업 비용이 크다.

**실용적 타협안 — 에디터에서 on/off하는 간이 토글:**

```cpp
// FeatureToggle.h — /Source/Core/ 에 위치
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Feature Toggles"))
class UFeatureToggleSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Features")
    bool bEnableCombat = true;

    UPROPERTY(Config, EditAnywhere, Category = "Features")
    bool bEnableEconomy = false;

    UPROPERTY(Config, EditAnywhere, Category = "Features")
    bool bEnableMonsterAI = true;

    UPROPERTY(Config, EditAnywhere, Category = "Features")
    bool bEnableSaveSystem = false;

    UPROPERTY(Config, EditAnywhere, Category = "Features")
    bool bEnableNPCTrade = false;
};
```

각 Subsystem에서 토글을 체크한다:

```cpp
void UEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!GetDefault<UFeatureToggleSettings>()->bEnableEconomy)
    {
        UE_LOG(LogTemp, Log, TEXT("Economy system disabled by FeatureToggle"));
        return; // 이 기능 전체 비활성화
    }

    // 정상 초기화 진행
    InitializePriceTables();
    RegisterMessageListeners();
}
```

`DefaultGame.ini`에서 한 줄 수정으로 시스템 단위 on/off:

```ini
[/Script/YourProject.FeatureToggleSettings]
bEnableCombat=True
bEnableEconomy=False
bEnableMonsterAI=True
bEnableSaveSystem=False
bEnableNPCTrade=False
```

---

## 7. 프로젝트 폴더 구조

### 7.1 Source 구조

```
/Source/
│
├── Core/                        ← 절대 변하지 않는 기반 (다른 모듈이 참조)
│   ├── CoreTypes.h              (공용 구조체: FItemStack 등)
│   ├── CoreInterfaces.h         (IDamageable, IInteractable 등)
│   ├── GameplayTagDefinitions.h (태그 상수 정의)
│   ├── MessageTypes.h           (이벤트 메시지 구조체들)
│   └── FeatureToggle.h          (기능 on/off 설정)
│
├── Systems/                     ← 각각 독립, 서로 #include 금지
│   ├── Combat/
│   │   ├── CombatComponent.h/.cpp
│   │   ├── CombatSubsystem.h/.cpp
│   │   └── AbilityDefinitions/
│   │
│   ├── Inventory/
│   │   ├── InventoryComponent.h/.cpp
│   │   ├── ItemDefinition.h/.cpp
│   │   └── ItemInstance.h/.cpp
│   │
│   ├── AIBehavior/
│   │   ├── MonsterAIController.h/.cpp
│   │   ├── MonsterSpawnSubsystem.h/.cpp
│   │   └── ItemConsumptionComponent.h/.cpp
│   │
│   ├── Economy/
│   │   ├── EconomySubsystem.h/.cpp
│   │   └── NPCTradeComponent.h/.cpp
│   │
│   ├── Progression/
│   │   ├── CollectionSubsystem.h/.cpp
│   │   └── UnlockSubsystem.h/.cpp
│   │
│   └── SaveLoad/
│       ├── SaveLoadSubsystem.h/.cpp
│       └── SaveGameData.h/.cpp
│
└── Game/                        ← 시스템을 조합하는 게임 로직
    ├── YourGameMode.h/.cpp
    ├── YourPlayerCharacter.h/.cpp
    └── YourGameInstance.h/.cpp
```

### 7.2 Content 구조

```
/Content/
│
├── Data/                        ← 데이터만 (코드 없음, 디자이너가 편집)
│   ├── Items/                   (UItemDefinition DataAsset들)
│   ├── Monsters/                (UMonsterDefinition DataAsset들)
│   ├── Abilities/               (스킬/어빌리티 DataAsset들)
│   └── Tables/                  (DataTable CSV: 밸런스 수치들)
│
├── Blueprints/                  ← Actor BP, Widget BP 등
│   ├── Characters/
│   ├── UI/
│   └── Effects/
│
└── TestMaps/                    ← 기능별 테스트 맵 (핵심!)
    ├── MAP_CombatOnly           (전투만 테스트)
    ├── MAP_InventoryFlow        (아이템 흐름만 테스트)
    ├── MAP_FullVerticalSlice    (핵심 루프 통합 테스트)
    └── MAP_PerformanceStress    (성능 스트레스 테스트)
```

### 7.3 황금 규칙

```
Systems/Combat/은 Systems/Inventory/를 #include하지 않는다.
Systems/Inventory/는 Systems/AIBehavior/를 #include하지 않는다.

시스템 간 통신은 오직 이 세 가지로만 한다:
  1. GameplayTag       (분류/식별)
  2. GameplayMessage   (이벤트 전파)
  3. Core Interfaces   (공용 인터페이스)
```

이 규칙을 어기는 순간 `#include` 하나가 들어가고, 그때부터 이터레이션 속도가 떨어지기 시작한다.

### 7.4 Build.cs 모듈 의존성

```csharp
// Core 모듈 — 의존성 최소
public class CoreModule : ModuleRules
{
    public CoreModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "GameplayTags",
            "GameplayMessageRuntime"  // 이벤트 버스
        });
    }
}

// Systems/Combat 모듈 — Core만 참조, 다른 Systems 참조 금지
public class CombatModule : ModuleRules
{
    public CombatModule(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine",
            "CoreModule",                  // ✅ Core만 참조
            "GameplayAbilities", "GameplayTags"
        });
        // ❌ "InventoryModule", "AIBehaviorModule" 절대 추가 금지
    }
}
```

---

## 8. 기능별 테스트 맵 전략

### 8.1 개념

테스트 맵은 **특정 시스템만 격리하여 검증하는 전용 레벨**이다. 이것이 이터레이션의 실질적인 속도를 결정한다.

### 8.2 테스트 맵 설계 원칙

```
MAP_CombatOnly
├── FeatureToggle: Combat=ON, Economy=OFF, SaveSystem=OFF
├── 환경: 50m × 50m 평지, 벽 4개
├── 배치: 플레이어 1, 더미 몬스터 3~5
├── 검증: 공격 느낌, 회피 타이밍, 데미지 공식
└── 기대 테스트 시간: 플레이 진입까지 5초 이내

MAP_InventoryFlow
├── FeatureToggle: Inventory=ON, Economy=OFF, Combat=OFF
├── 환경: 작은 방, 아이템 스폰 포인트 10개
├── 배치: 플레이어 1, 인벤토리 UI, 저장소 Actor 1
├── 검증: 줍기, 저장, 이동, 버리기, 무게 제한
└── 기대 테스트 시간: 플레이 진입까지 3초 이내

MAP_FullVerticalSlice
├── FeatureToggle: ALL=ON
├── 환경: 500m × 500m 필드, 거점 1개
├── 배치: 플레이어, 카라반, 몬스터 군집, NPC 1
├── 검증: 핵심 게임 루프 전체
└── 기대 테스트 시간: 플레이 진입까지 10초 이내
```

### 8.3 테스트 맵 전용 GameMode

```cpp
UCLASS()
class ATestGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    virtual void StartPlay() override
    {
        Super::StartPlay();

        // 테스트 편의 기능
        SpawnTestItems();
        SetupInvulnerablePlayer();  // 전투 밸런스 아닌 것 테스트 시
    }

    UFUNCTION(Exec)
    void SpawnMonsters(int32 Count); // 콘솔 명령으로 즉시 몬스터 소환

    UFUNCTION(Exec)
    void GiveAllItems(); // 콘솔 명령으로 전체 아이템 획득

    UFUNCTION(Exec)
    void DestroyCaravan(); // 콘솔 명령으로 카라반 강제 파괴
};
```

---

## 9. 이벤트 흐름 설계 예시

### 9.1 "카라반 파괴 → 몬스터 아이템 섭취" 전체 흐름

```
[1] 몬스터가 카라반 공격
    → CombatComponent::ApplyDamage()
    → HealthComponent의 HP 감소

[2] 카라반 HP가 0 도달
    → HealthComponent가 OnDeath 델리게이트 발화
    → CaravanBehaviorComponent::OnCaravanDestroyed() 호출

[3] 카라반이 이벤트 발행
    → BroadcastMessage(TAG_Event_Caravan_Destroyed, FCaravanDestroyedMessage)

[4] 여러 시스템이 동시에 수신:
    ├── ItemDropSubsystem: 아이템을 필드에 물리적으로 스캐터
    │   → 각 아이템마다 BroadcastMessage(TAG_Event_Item_Dropped)
    │
    ├── MonsterPerceptionRelay: 반경 내 몬스터에게 Stimulus 전파
    │   → 몬스터 BT/ST가 "아이템 줍기" 태스크로 전환
    │
    ├── UISubsystem: "카라반 파괴!" 경고 위젯 표시
    │
    └── AI Director: 위험도 스케일링 조정

[5] 몬스터가 아이템에 도달하여 섭취
    → ItemConsumptionComponent: 아이템 태그 확인 → 매칭 GE 적용
    → BroadcastMessage(TAG_Event_Monster_ConsumedItem)

[6] 섭취 이벤트 수신:
    ├── MonsterVisualUpdater: 외형 변화 (머티리얼 파라미터)
    ├── UISubsystem: "몬스터가 [골드 광석]을 먹었습니다!" 표시
    └── CollectionSubsystem: 도감 업데이트 (첫 섭취 기록)
```

모든 시스템은 **메시지만 주고받으며, 서로의 존재를 모른다**.

### 9.2 시스템 추가/제거 시나리오

```
시나리오: "도감 시스템이 아직 없다"
→ CollectionSubsystem이 존재하지 않음
→ TAG_Event_Monster_ConsumedItem 메시지는 여전히 발행됨
→ 아무도 안 들으면 그냥 무시됨
→ 나머지 시스템: 영향 없음 ✅

시나리오: "사운드 시스템을 새로 추가하고 싶다"
→ SoundSubsystem 생성
→ Initialize에서 TAG_Event_Caravan_Destroyed에 리스너 등록
→ 기존 코드 수정: 0줄 ✅
```

---

## 10. 실제 이터레이션 시나리오

### 시나리오 A — 데이터 수정 (컴파일 0회)

```
"카라반 파괴 시 아이템이 폭발적으로 튀는 게 재미없다"
  → /Content/Data/Tables/ItemScatter.csv의 ScatterForce 값만 수정
  → 에디터에서 MAP_FullVerticalSlice 바로 플레이
  → 30초 만에 느낌 확인
  → C++ 컴파일: 0회
```

### 시나리오 B — 단일 시스템 전면 재설계

```
"몬스터 아이템 섭취 AI를 전면 재설계하고 싶다"
  → Systems/AIBehavior/ 폴더만 통째로 리팩터
  → 다른 시스템은 TAG_Event_Monster_ConsumedItem 메시지만 기다리는 중
  → 메시지 구조체(FMonsterConsumedItemMessage)만 동일하면 됨
  → 나머지 시스템 수정: 0줄
```

### 시나리오 C — 핵심 시스템 교체

```
"전투 시스템을 GAS에서 커스텀으로 바꿔보자"
  → FeatureToggle에서 bEnableCombat = false
  → Systems/Combat/ 폴더를 새 구현으로 교체
  → 외부 인터페이스(IDamageable)와 메시지 구조체만 동일하면 됨
  → 인벤토리, 카라반, 경제: 아무것도 안 바뀜
```

### 시나리오 D — 기능 격리 테스트

```
"경제 시스템 아직 필요 없다, 이번 주는 AI만 테스트"
  → MAP_CombatOnly 맵에서 FeatureToggle Economy=false로 플레이
  → 경제 코드가 아예 초기화 안 됨
  → 로딩 빠름, 테스트 집중
```

### 시나리오 E — 새 시스템 추가

```
"날씨 시스템을 실험해보고 싶다"
  → Systems/Weather/ 폴더 생성
  → UWeatherSubsystem : UWorldSubsystem 생성
  → FeatureToggle에 bEnableWeather 추가
  → 기존 시스템에서 TAG_Event_Weather_Changed를 구독하고 싶은 시스템만 리스너 추가
  → 기존 코드 수정: 0줄 (리스너 추가는 각 시스템의 자발적 선택)
```

---

## 11. 통신 수단 선택 가이드

### 11.1 세 가지 통신 수단 비교

| 수단 | 범위 | 용도 | 예시 |
|------|------|------|------|
| **GameplayMessage** | 글로벌 (월드 전체) | 시스템 → 시스템 | "카라반 파괴됨", "아이템 드랍됨" |
| **Delegate / Event** | 로컬 (해당 Actor/Component) | Component → 직접 소유자 | "HP 변경됨" → 해당 Actor의 HP바 갱신 |
| **Interface 함수 호출** | 1:1 (특정 Actor) | 확인된 대상에 직접 명령 | "이 Actor에 데미지 적용" |

### 11.2 선택 기준 플로우차트

```
이벤트 발생 시:
│
├── "누가 들을지 모른다 / 여러 시스템이 반응해야 한다"
│   → GameplayMessage (이벤트 버스)
│
├── "이 Actor의 다른 Component가 알아야 한다"
│   → Delegate / Multicast Delegate
│
└── "특정 대상에게 직접 행동을 시켜야 한다"
    → Interface 함수 호출 (Cast<IInterface>)
```

---

## 12. 프로토타입 초기 셋업 체크리스트

### 12.1 Day 0 — 프로젝트 생성 직후 (2~3시간)

- [ ] UE5 프로젝트 생성 (C++, World Partition 기본 활성화)
- [ ] Source 모듈 분리: Core, Systems/*, Game
- [ ] `Core/GameplayTagDefinitions.h` 생성 — 초기 태그 구조 정의
- [ ] `Core/MessageTypes.h` 생성 — 기본 메시지 구조체 정의
- [ ] `Core/CoreInterfaces.h` 생성 — IDamageable, IInteractable
- [ ] `Core/FeatureToggle.h` 생성 — 초기 토글 설정
- [ ] GameplayMessageSubsystem 플러그인 활성화 확인

### 12.2 Day 1 — 데이터 레이어 (3~4시간)

- [ ] `UItemDefinition` DataAsset 클래스 생성
- [ ] 테스트용 DataTable 1개 생성 (몬스터 스폰 데이터)
- [ ] 테스트용 DataAsset 3~5개 생성 (임시 아이템)
- [ ] GameplayTag 초기 세트 등록 (.ini)

### 12.3 Day 2~3 — 첫 번째 테스트 맵 (6~8시간)

- [ ] `MAP_CombatOnly` 테스트 맵 생성 (50m × 50m 평지)
- [ ] 최소 플레이어 캐릭터 (이동 + 기본 공격)
- [ ] 최소 더미 몬스터 (HP만 있는 Actor)
- [ ] 테스트용 GameMode (콘솔 명령 포함)
- [ ] **첫 번째 이터레이션 사이클 완료 확인**

---

## 13. 흔한 실수와 대응

### 13.1 "나중에 분리하면 되지"

**현실**: 모놀리식으로 시작하면 절대 분리 안 한다. 기능이 추가될수록 의존성이 기하급수적으로 증가하여, 분리 비용이 새로 만드는 것보다 커지는 시점이 빠르게 온다.

**대응**: Day 0에 모듈 구조를 깔아두는 것이 전체 프로젝트에서 가장 ROI가 높은 작업이다.

### 13.2 "이벤트 버스가 디버깅하기 어렵다"

**현실**: 맞다. "누가 이 메시지를 보냈고, 누가 받았는가?"를 추적하기 어려울 수 있다.

**대응**: 메시지 발행/수신 시 로그를 남기는 래퍼 함수를 만든다.

```cpp
// 디버그용 래퍼
template<typename TMessage>
void BroadcastWithLog(UWorld* World, FGameplayTag Tag, const TMessage& Msg)
{
#if !UE_BUILD_SHIPPING
    UE_LOG(LogGameplayMsg, Verbose,
        TEXT("[MSG] Broadcast: %s"), *Tag.ToString());
#endif
    UGameplayMessageSubsystem::Get(World).BroadcastMessage(Tag, Msg);
}
```

### 13.3 "컴포넌트가 너무 많아져서 관리가 안 된다"

**현실**: Actor에 10개 이상 컴포넌트가 붙으면 초기화 순서 문제가 발생할 수 있다.

**대응**: 컴포넌트 간 의존이 있으면 `InitializeComponent()` 단계에서 `UGameFrameworkComponentManager`의 Init State 체인을 사용하거나, 단순하게 `PostInitializeComponents()`에서 한 번에 연결한다.

### 13.4 "FeatureToggle이 실제 프로덕션에서도 필요한가?"

**현실**: 프로토타입/알파 단계에서는 필수. 베타 이후에는 모든 기능이 ON이므로 토글 체크 오버헤드가 무의미해진다.

**대응**: `#if WITH_EDITOR` 또는 `#if !UE_BUILD_SHIPPING`으로 토글 체크를 개발 빌드에서만 동작하게 제한할 수 있다.

---

## 14. Vertical Slice 역순 구축법

### 14.1 개념

일반적인 접근은 "밑에서 위로 쌓는 것"이다. 그러나 1인 개발에서 더 효과적인 방법이 있다.

**최종 경험의 가장 짧은 버전을 먼저 정의하고, 그 안에서 필요한 시스템을 역추적하여 구현한다.**

### 14.2 예시

```
목표: "30초 안에 핵심 재미를 검증할 수 있는 시퀀스"

"플레이어가 작은 평지에서 몬스터 3마리를 잡고,
 아이템 5개를 카라반에 싣고,
 돌아가는 길에 몬스터가 카라반을 공격해서 아이템이 쏟아지고,
 몬스터가 그걸 먹어치우는 30초"
```

이 시퀀스를 돌리는 데 필요한 것만 만든다:

- 플레이어 캐릭터 (이동 + 공격): 필요 ✅
- 카라반 AI (추적): 필요 ✅
- 인벤토리 (줍기 + 저장): 필요 ✅
- 카라반 HP + 파괴 + 아이템 낙하: 필요 ✅
- 몬스터 AI (공격 + 아이템 섭취): 필요 ✅
- 경제 시스템: 불필요 ❌
- 세이브/로드: 불필요 ❌
- World Partition: 불필요 ❌
- NPC 거래: 불필요 ❌
- 도감: 불필요 ❌

이 30초가 재미있으면 게임은 된다. 재미없으면 월드를 아무리 넓혀도 안 된다.

### 14.3 반복 루프

```
[Iteration 1] 30초 Vertical Slice 구현 → 테스트 → "전투 느낌이 밋밋하다"
    → DataTable에서 데미지/넉백 수치 조정 (컴파일 0회)
    → 재테스트

[Iteration 2] "카라반이 너무 빨리 부서진다"
    → 카라반 HP DataAsset 수정 (컴파일 0회)
    → 재테스트

[Iteration 3] "몬스터가 아이템 먹는 게 안 보인다"
    → AIBehavior 시스템만 수정 → 섭취 애니메이션 추가
    → 다른 시스템 영향: 0

[Iteration N] 핵심 재미가 검증됨
    → 그제서야 경제, NPC, 월드 확장을 시작
```

---

## 15. 요약 — 핵심 구조 한 장 정리

```
┌─────────────────────────────────────────────────────────────┐
│                        CORE MODULE                          │
│  GameplayTags │ MessageTypes │ Interfaces │ FeatureToggle   │
└───────────┬─────────────────────────────────────┬───────────┘
            │ (모든 시스템이 Core만 참조)           │
            ▼                                      ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│  Systems/Combat  │  │ Systems/Inventory│  │ Systems/AIBehavior│
│  ├── Component   │  │ ├── Component    │  │ ├── Controller    │
│  └── Subsystem   │  │ └── DataAssets   │  │ └── Subsystem     │
└────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
         │                     │                      │
         ▼                     ▼                      ▼
    ┌──────────────────────────────────────────────────────┐
    │           GameplayMessageSubsystem (이벤트 버스)       │
    │                                                       │
    │  TAG_Event_Caravan_Destroyed ←→ 모든 관심 시스템       │
    │  TAG_Event_Item_Dropped      ←→ 모든 관심 시스템       │
    │  TAG_Event_Monster_Consumed  ←→ 모든 관심 시스템       │
    └──────────────────────────────────────────────────────┘
         ▲                     ▲                      ▲
         │                     │                      │
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│ Systems/Economy  │  │ Systems/SaveLoad │  │Systems/Progression│
│  └── Subsystem   │  │ └── Subsystem    │  │ └── Subsystem     │
└──────────────────┘  └──────────────────┘  └──────────────────┘

         │  Content/Data/ (DataAsset, DataTable, CSV)
         │  → 밸런스 수정 시 컴파일 없이 즉시 반영
         │
         │  Content/TestMaps/ (기능별 격리 테스트 맵)
         │  → 기능 단위로 빠르게 검증
```

**세 줄 요약**:

1. **데이터는 코드 밖으로** — DataAsset, DataTable, GameplayTag로 밸런스를 에디터에서 조정
2. **시스템은 메시지로만 대화** — GameplayMessageSubsystem으로 직접 참조 제거
3. **기능은 조합으로 구성** — Component + Interface + FeatureToggle로 독립적 on/off

---

## 부록 A. GameplayMessageSubsystem 설정

### A.1 플러그인 활성화

프로젝트의 `.uproject` 파일 또는 에디터 Plugins 메뉴에서 `GameplayMessageRuntime` 플러그인이 활성화되어 있는지 확인한다. Lyra 기반 프로젝트라면 이미 포함되어 있다.

독립 프로젝트라면 `Build.cs`에 다음을 추가한다:

```csharp
PublicDependencyModuleNames.Add("GameplayMessageRuntime");
```

### A.2 커스텀 경량 이벤트 버스 (대안)

GameplayMessageSubsystem을 사용할 수 없는 환경이라면, 최소한의 이벤트 버스를 직접 구현할 수 있다:

```cpp
UCLASS()
class USimpleEventBus : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
        FOnGameEvent, FGameplayTag, EventTag, UObject*, Payload);

    UPROPERTY(BlueprintAssignable)
    FOnGameEvent OnGameEvent;

    UFUNCTION(BlueprintCallable)
    void BroadcastEvent(FGameplayTag EventTag, UObject* Payload = nullptr)
    {
        OnGameEvent.Broadcast(EventTag, Payload);
    }
};
```

---

## 부록 B. 참고 자료

- **Lyra Starter Game** — Epic Games의 UE5 공식 샘플. ModularGameplay, GameFeature Plugin, GameplayMessageSubsystem의 실제 프로덕션 구현 참고
- **GASDocumentation (GitHub: tranek)** — Gameplay Ability System 문서. UE5.3 기준
- **Tom Looman — UE5 C++ Save System** — 오픈 월드 세이브/로드 아키텍처
- **X157 Dev Notes** — ModularGameplay, GameFeature Plugin 심층 분석
- **Unreal Community Wiki — GameplayMessageSystem** — 이벤트 버스 사용법 튜토리얼
