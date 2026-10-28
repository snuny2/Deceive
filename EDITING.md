# 수정 가이드

지금까지 만든 것에서 **무엇을 어디서 바꿀 수 있는지** 정리한 문서입니다.
구조 설명은 [README.md](README.md), 진행 경과는 [HANDOFF.md](HANDOFF.md)에 있습니다.

---

## 0. 먼저 해둘 것 — 블루프린트 자식 만들기

지금은 레벨에 배치된 게 아무것도 없고 GameMode가 런타임에 스폰합니다. 그래서
**디테일 패널에서 값을 만지려면 블루프린트 자식이 필요합니다.** 한 번만 해두면
그 뒤로는 전부 클릭으로 수정됩니다.

Content Browser → 우클릭 → Blueprint Class → 아래 클래스를 상속:

| 만들 것 | 상속할 클래스 | 연결하는 곳 |
| --- | --- | --- |
| `BP_SpriteCharacter` | `SpriteCharacter` | `BP_DeceiveGameMode` → Default Pawn Class |
| `BP_CrossAlley` | `CrossAlley` | `BP_DeceiveGameMode` → Alley Class |
| `BP_InteractableBox` | `InteractableBox` | `BP_DeceiveGameMode` → Test Box Class |
| `BP_DeceiveHUD` | `DeceiveHUD` | `BP_DeceiveGameMode` → HUD Class |
| `BP_DeceiveGameMode` | `DeceiveGameMode` | Project Settings → Maps & Modes |

마지막으로 **Project Settings → Maps & Modes → Default GameMode** 를
`BP_DeceiveGameMode` 로 바꾸면 끝입니다.

> 블루프린트를 안 만들고 C++ 생성자 값을 직접 고쳐도 됩니다. 그 경우 매번
> 에디터를 닫고 빌드해야 합니다 (헤더를 건드리면 핫리로드가 안 됨).

---

## 1. 텍스처

### 교체하는 곳

| 대상 | 클래스 | 프로퍼티 |
| --- | --- | --- |
| 캐릭터 | `SpriteCharacter` | `Sprite → Sprite Texture` |
| 바닥 | `CrossAlley` | `Alley\|Texture → Floor Texture` |
| 벽 | `CrossAlley` | `Alley\|Texture → Wall Texture` |
| 상자 | `InteractableBox` | `Box → Box Texture` |

비워두면 `/Engine/EngineResources/DefaultTexture` 가 플레이스홀더로 들어갑니다.
지금 화면이 흐릿한 회색 얼룩인 게 그것입니다.

### 텍스처 임포트 설정 (HD-2D에 중요)

PNG를 Content Browser에 끌어다 놓은 뒤, 더블클릭해서:

| 항목 | 값 | 이유 |
| --- | --- | --- |
| Filter | **Nearest** | 픽셀아트가 뭉개지지 않음 |
| Compression Settings | **UserInterface2D (RGBA)** | 알파 보존, 블록 압축 아티팩트 없음 |
| Mip Gen Settings | **NoMipmaps** | 멀리서 흐려지는 것 방지 |
| sRGB | 체크 유지 | 색상 텍스처 기본 |

캐릭터 스프라이트는 **알파 채널이 있는 PNG** 여야 합니다. 머티리얼이 마스크드라
알파가 잘리는 부분이 투명해집니다.

### 지금의 한계 — 타일링이 안 됨

벽 하나에 텍스처 **한 장이 통째로 늘어납니다.** 2400 길이 벽에 작은 벽돌 텍스처를
넣으면 그만큼 늘어나 뭉개집니다.

타일링하려면 머티리얼을 직접 만들어야 합니다:

1. Content Browser → Material 생성
2. `TexCoord` → `Multiply` (B에 반복 횟수) → `TextureSample` 의 UVs 에 연결
3. `TextureSample` 을 **Texture Parameter** 로 만들고 이름을 정확히 `SpriteTexture` 로
4. Base Color 에 연결, Shading Model = **Unlit**, Blend Mode = **Opaque** (벽/바닥)
5. `CrossAlley` 의 `Base Material` 에 그 머티리얼을 지정

파라미터 이름이 `SpriteTexture` 가 아니면 텍스처 교체가 먹지 않습니다.

### 머티리얼 기본값

| 대상 | 기본 머티리얼 | 성격 |
| --- | --- | --- |
| 캐릭터 | `/Paper2D/MaskedUnlitSpriteMaterial` | 컷아웃, 언릿 |
| 벽·바닥·상자 | `/Paper2D/OpaqueUnlitSpriteMaterial` | 불투명, 언릿 |

전부 **언릿**이라 레벨에 라이트가 없어도 보입니다. 나중에 라이팅을 넣으려면
`Lit` 계열로 바꾸면 됩니다 (`DefaultSpriteMaterial`, `OpaqueLitSpriteMaterial` 등).

---

## 2. 대사

### 구조

`InteractableBox` 의 `Dialogue` (`FDialogueData`) 하나에 전부 들어 있습니다.

| 필드 | 역할 |
| --- | --- |
| `Speaker` | 창 위 이름 (금색). **비우면 이름 줄이 사라짐** |
| `ChoiceId` | 선택 기록용 이름. **질문마다 다르게.** 비우면 기록은 되지만 나중에 찾을 수 없음 |
| `Lines` | 도입부. **배열 한 칸 = 창 하나** |
| `Question` | 선택지 질문. **비우면 선택 단계 없이 도입만 하고 끝남** |
| `ChoiceA.Label` / `ChoiceB.Label` | 선택지 문구 (앞의 "A. " "B. " 는 자동으로 붙음) |
| `ChoiceA.ResultLines` / `ChoiceB.ResultLines` | 선택 후 나올 줄들 |

진행: **도입 → 질문/선택지 → 선택별 결과 → 닫힘**

### 자주 하는 수정

**선택지 없는 평범한 대사로:** `Question` 을 비우면 됩니다. `Lines` 만 재생되고 닫힙니다.

**질문부터 바로 시작:** `Lines` 를 비우면 열자마자 질문이 뜹니다.

**한쪽 선택은 아무 말 없이 끝내기:** 그 선택의 `ResultLines` 를 비우면 즉시 닫힙니다.

**줄 길이:** 알아서 접히므로 길게 써도 됩니다. 다만 한 칸이 너무 길면 창이 세로로
커지니, 두세 줄 분량에서 배열을 나누는 게 보기 좋습니다.

### 새 상호작용 대상 만들기

가장 쉬운 방법은 `BP_InteractableBox` 를 복제해서 `Dialogue` 만 바꾸는 것입니다.

C++ 로 새 종류를 만들려면:

```cpp
class AMyThing : public AActor, public IInteractable
{
    GENERATED_BODY()
public:
    virtual void Interact_Implementation(AActor* InteractingActor) override;

    UPROPERTY(EditAnywhere, Category = "Dialogue")
    FDialogueData Dialogue;
};

void AMyThing::Interact_Implementation(AActor* InteractingActor)
{
    if (ASpriteCharacter* Player = Cast<ASpriteCharacter>(InteractingActor))
    {
        Player->StartDialogue(Dialogue);
    }
}
```

`IInteractable` 을 구현하기만 하면 근접 감지와 "F" 프롬프트는 자동으로 붙습니다.
`Interact` 는 `BlueprintNativeEvent` 라 블루프린트에서 오버라이드해도 됩니다.

### 배치 위치

지금은 `DeceiveGameMode::InitGame` 이 교차점 `(0, 0, 75)` 에 하나만 스폰합니다.
여러 개를 두려면 레벨에 `BP_InteractableBox` 를 직접 끌어다 놓는 게 낫습니다
(이건 레벨 애셋이므로 에디터에서만 가능).

---

## 3. 대사창·프롬프트 모양 — `DeceiveHUD`

| 프로퍼티 | 기본값 | 역할 |
| --- | --- | --- |
| `Interact Prompt` | `조사한다` | 대상이 문구를 안 정했을 때의 대체값 |

프롬프트는 **선택지 패널과 같은 자리**에 뜹니다. 문구는 대상이 정합니다:
`InteractableBox → Interact Prompt`(기본 "조사한다"),
`AreaTransition → Interact Prompt`(기본 "이동한다").
| `Dialogue Width Fraction` | 0.8 | 창 가로폭 (화면 대비 비율) |
| `Dialogue Text Scale` | 1.3 | 본문 글자 크기 |
| `Dialogue Background Color` | 검정 80% | 창 배경 |
| `Dialogue Speaker Color` | 금색 | 화자명 색 |
| `Choice Color` | 하늘색 | 선택지 색 |
| `Dialogue Hint` | `F` | 우하단 힌트 (진행 중) |
| `Choice Hint` | `↑↓ 선택  F 확정` | 우하단 힌트 (선택 대기 중) |
| `Choice Cursor` | `▶ ` | 선택 커서 표시 |
| `Choice Dim Color` | 회색 | 고르지 않은 선택지 색 |

선택지는 **대사창 바로 위, 오른쪽 끝에 맞춘 별도 패널**에 그려집니다. 패널 폭은
선택지 글자 길이에 맞춰 자동으로 정해집니다.

> 지금 창 배경이 잘 안 보이는 건 화면 아래쪽이 검은 공백이라 검정 위에 검정이라
> 그렇습니다. 텍스처와 라이팅이 들어가면 구분됩니다. 당장 띄우고 싶으면
> `Dialogue Background Color` 의 알파를 올리거나 색을 바꾸면 됩니다.

---

## 3-2. 맵 영역과 출입구

한 번에 한 영역만 존재합니다 (`Alley` / `Building` / `NextStreet`).
방향은 화면 기준으로 **위 = +X, 왼쪽 = -Y** 입니다.

| 바꾸고 싶은 것 | 어디서 |
| --- | --- |
| 포탈 질문·선택지 문구 | `AreaTransition → Dialogue` (비우면 목적지에 맞춰 자동) |
| 건물 문 위치 | `CrossAlley → Door Center Y` (기본 -700, -Y 팔 위에서의 위치) |
| 건물 문 폭 | `CrossAlley → Door Width` (기본 300). 트리거 크기도 같이 따라감 |
| 다음 거리 크기 | `DeceiveGameMode → Next Street Width / Depth` (기본 2000 × 500) |
| 방 크기 | `RoomArea → Room Width` (가로) / `Room Depth` (깊이) |
| 방 문 폭 | `RoomArea → Door Width` |
| 방 텍스처 | `RoomArea → Floor Texture` / `Wall Texture` |

`RoomArea` 의 문은 항상 **카메라 쪽(-X) 벽**에 있고, 그 벽은 렌더링이 꺼져 있어
방 안이 보입니다.

포탈은 투명하지만 **목적지 색 디버그 박스로 그려집니다** — 초록 = 건물,
하늘색 = 다음 거리, 노랑 = 골목길. 벽 안에 있어도 보입니다.
끄려면 `AreaTransition.h` 의 `bShowDebugVolume` 를 `false` 로.

새 영역을 추가하려면 코드가 필요합니다 — README 의 "영역 추가하기" 참고.

## 4. 골목길 모양 — `CrossAlley`

| 프로퍼티 | 기본값 | 역할 |
| --- | --- | --- |
| `Corridor Width` | 400 | 복도 폭 |
| `Arm Length` | 1200 | 교차점 중심 → 갈래 끝 |
| `Wall Height` | 600 | 벽 높이 |
| `Wall Thickness` | 100 | 벽 두께 |
| `Floor Thickness` | 20 | 바닥 두께 |
| `b Hide Camera Side Walls` | 켜짐 | 카메라 쪽 벽 5개를 렌더링만 끔 (충돌은 유지) |

값을 바꾸면 `OnConstruction` 이 다시 계산해 **에디터에서 즉시 반영**됩니다.

`bHideCameraSideWalls` 를 끄면 벽이 다 보이지만 카메라가 벽 안을 들여다보게 됩니다.
카메라 각도를 많이 바꿀 거라면 이 숨김 규칙도 같이 손봐야 합니다 (`CrossAlley.cpp`
의 `SignAlong < 0` / `SignSide < 0` 조건).

---

## 5. 카메라

**디테일 패널에 직접 노출된 프로퍼티가 없습니다.** 값이 `SpriteCharacter` 생성자에
박혀 있습니다. 두 가지 방법이 있습니다.

**방법 1 — 블루프린트에서 컴포넌트를 선택** (빌드 불필요, 권장)

`BP_SpriteCharacter` 를 열고 컴포넌트 트리에서:

| 컴포넌트 | 프로퍼티 | 현재값 |
| --- | --- | --- |
| `CameraBoom` | Target Arm Length | 2700 |
| `CameraBoom` | Rotation (Pitch) | -25 |
| `Camera` | Field Of View | 40 |

**방법 2 — C++ 수정** (`SpriteCharacter.cpp` 생성자, 빌드 필요)

```cpp
CameraBoom->TargetArmLength = 2700.f;
CameraBoom->SetWorldRotation(FRotator(-25.f, 0.f, 0.f));
Camera->FieldOfView = 40.f;
```

> 카메라 Yaw 는 0 고정입니다 (`SetUsingAbsoluteRotation(true)`). **Yaw 를 바꾸면
> 벽 숨김 규칙과 이동 방향이 둘 다 어긋납니다** — 둘 다 "카메라는 -X 에서 +X 를
> 본다"를 전제로 하고 있습니다.

---

## 6. 캐릭터

| 프로퍼티 | 기본값 | 역할 |
| --- | --- | --- |
| `Sprite → Sprite Width` | 120 | 스프라이트 가로 (월드 단위) |
| `Sprite → Sprite Height` | 180 | 스프라이트 세로 |
| `Sprite → Sprite Texture` | 없음 | 외형 |
| `Interaction → Interaction Radius` | **60** | 상호작용 감지 범위. **플레이어 중심 기준**이라 캡슐 반지름 34 를 빼면 실제 여유는 약 26 |

`InteractionRadius` 는 `BeginPlay` 에서 스피어 컴포넌트에 덮어씌워집니다. 그러니
**스피어 컴포넌트의 Sphere Radius 를 직접 고치지 말고 이 값을 고치세요.**

이동 속도는 노출돼 있지 않습니다 — `SpriteCharacter.cpp` 생성자의
`GetCharacterMovement()->MaxWalkSpeed = 350.f;` 이거나, 블루프린트에서
`CharacterMovement` 컴포넌트의 Max Walk Speed 를 바꾸면 됩니다.

---

## 7. 입력 키

**디테일 패널에서 못 바꿉니다.** `SpriteCharacter.cpp` 의 `EnsureInputObjects()` 에
코드로 박혀 있습니다 (Enhanced Input 애셋을 만들 수 없어서 런타임 생성).

```cpp
MapMove(EKeys::W, true, false);          // 전진
MappingContext->MapKey(InteractAction, EKeys::F);
MappingContext->MapKey(NavigateUpAction, EKeys::Up);
MappingContext->MapKey(NavigateDownAction, EKeys::Down);
```

| 액션 | 현재 키 |
| --- | --- |
| 이동 | WASD, 방향키, 게임패드 왼쪽 스틱 |
| 상호작용 / 다음 줄 | F, 게임패드 아래 버튼 |
| 선택지 커서 이동 | ↑ ↓, 게임패드 D-Pad 위/아래 |
| 선택 확정 | F (상호작용 키와 같음) |

---

## 7-2. 선택 기록

선택한 답은 `UDeceiveGameInstance` 에 쌓입니다. 바꿀 것은 두 가지입니다.

**질문마다 `ChoiceId` 를 주세요.** `InteractableBox → Dialogue → Choice Id` 입니다.
같은 ID 로 다시 답하면 이전 기록을 덮어씁니다. 비워두면 저장은 되지만
`TryGetChoice` 로 찾을 수 없어 분기에 쓸 수 없습니다.

**요약 화면 문구와 색**은 `DeceiveHUD` 에서:

| 프로퍼티 | 기본값 |
| --- | --- |
| `Summary Title` | `선택 기록` |
| `Summary Empty Text` | `기록된 선택이 없습니다.` |
| `Summary Text Scale` | 1.3 |
| `Summary Background Color` | 거의 검정 94% |

**요약은 게임 중엔 안 뜹니다.** `EndGame()` 이 불린 뒤에만 뜹니다. 아직 엔딩이
없어서 부르는 곳이 없으니, 테스트할 때는 콘솔(`~`)에 `EndGame` 을 치세요.

## 8. 코드를 고쳐야만 되는 것들

| 하고 싶은 것 | 고칠 곳 |
| --- | --- |
| 입력 키 변경 | `SpriteCharacter.cpp` → `EnsureInputObjects()` |
| 선택지 3개 이상 | `DeceiveDialogueTypes.h` → `TArray<FDialogueChoice>` 로 변경 후 `MoveSelection` 의 clamp 범위 수정 |
| 선택지 패널 위치 | `DeceiveHUD.cpp` → `DrawChoicePanel()` |
| 대사 중 이동 허용 | `SpriteCharacter.cpp` → `Move()` 의 `IsDialogueActive()` 체크 제거 |
| 벽 숨김 규칙 | `CrossAlley.cpp` → `OnConstruction()` 의 부호 조건 |
| 플레이어 시작 위치 | `CrossAlley.cpp` → `GetEntryLocation()` 의 기본 반환값 |
| 영역 도착 지점 | 각 영역의 `GetEntryLocation()` |
| 새 영역 추가 | `EDeceiveArea` + 새 `ADeceiveArea` 상속 클래스 + `SpawnArea` 분기 |
| 테스트 박스 위치 | `CrossAlley.cpp` → `SpawnContents()` 의 `(0, 0, 75)` |
| 엔딩에서 요약 띄우기 | `UDeceiveGameInstance::EndGame()` 을 호출하면 됨 |

### 빌드가 필요한지 판단

| 바꾼 것 | 필요한 작업 |
| --- | --- |
| 블루프린트 / 디테일 패널 값 | 없음, 바로 Play |
| `.cpp` 만 수정 | Live Coding (Ctrl+Alt+F11) 가능 |
| `.h` 수정 (프로퍼티 추가 등) | **에디터 종료 후 전체 빌드** |
