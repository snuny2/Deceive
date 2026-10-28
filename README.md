# Deceive — HD-2D 프로토타입

십자 골목길 + 빌보드 스프라이트 캐릭터 1명. 레벨/머티리얼 애셋 없이 C++ 로만 구성되어
있어서, 빈 레벨 하나만 있으면 바로 플레이됩니다.

## 처음 실행하기

1. `Deceive.uproject` 우클릭 → **Generate Visual Studio project files**
2. `Deceive.sln` 빌드 (Development Editor / Win64), 또는 `.uproject` 를 더블클릭해
   모듈 리빌드 프롬프트에 Yes
3. 에디터에서 **File → New Level → Empty Level** 로 레벨을 만들고 `Content/Maps/Alley`
   같은 경로에 저장
4. **Project Settings → Maps & Modes** 에서 Editor/Game Default Map 을 그 레벨로 지정
   (GameMode 는 `DeceiveGameMode` 로 이미 설정돼 있음)
5. Play. WASD / 방향키 / 게임패드 왼쪽 스틱으로 이동, **F** 로 상호작용

레벨에 아무것도 넣지 않아도 `ADeceiveGameMode::InitGame` 이 골목길과 스폰 지점을
런타임에 생성합니다.

빠르게 확인만 하고 싶으면 레벨을 만들지 않고 엔진 기본 맵으로 바로 실행해도 됩니다:
`UnrealEditor-Cmd.exe Deceive.uproject /Engine/Maps/Entry -game`

단 `/Engine/Maps/Entry` 에는 원점에 엔진 자체 PlayerStart 가 하나 들어 있어서, 둘 중
어느 쪽에서 시작할지 매번 달라집니다. 직접 만든 빈 레벨에는 없는 문제입니다.

## 텍스처 교체

캐릭터는 Paper2D 의 `MaskedUnlitSpriteMaterial`(컷아웃), 벽과 바닥은
`OpaqueUnlitSpriteMaterial`(불투명) 을 기본으로 씁니다. 텍스처 파라미터 이름은 둘 다
`SpriteTexture` 입니다. 언릿이라 레벨에 라이트가 없어도 보입니다.

텍스처를 지정하지 않으면 `/Engine/EngineResources/DefaultTexture` 가 플레이스홀더로
들어갑니다. Paper2D 기본 플레이스홀더(흐릿한 원)보다 형상 파악이 쉬워서입니다.

| 대상 | 바꾸는 곳 |
| --- | --- |
| 캐릭터 | `ASpriteCharacter::SpriteTexture` (+ `SpriteWidth` / `SpriteHeight`) |
| 바닥 | `ACrossAlley::FloorTexture` |
| 벽 | `ACrossAlley::WallTexture` |

에디터에서 만지려면 두 클래스의 블루프린트 자식을 만드는 게 가장 편합니다.

- Content Browser → Blueprint Class → `SpriteCharacter` 상속 → `BP_SpriteCharacter`.
  디테일 패널에서 Sprite Texture 지정 후, `DeceiveGameMode` 블루프린트 자식의
  Default Pawn Class 를 `BP_SpriteCharacter` 로 지정.
- 같은 방식으로 `CrossAlley` 상속 블루프린트를 만들고 `DeceiveGameMode` 의
  `AlleyClass` 에 지정.

머티리얼을 직접 만들려면 Texture Sample(파라미터 이름 `SpriteTexture`) → Base Color,
Alpha → Opacity Mask, Shading Model = Unlit, Blend Mode = Masked 로 두고 각 클래스의
`BaseMaterial` 에 꽂으면 됩니다.

## 입력 (Enhanced Input)

`InputAction` / `InputMappingContext` 는 `.uasset` 이라 만들 수 없으므로,
`ASpriteCharacter::EnsureInputObjects()` 가 `NewObject` 로 런타임에 생성합니다.
`Config/DefaultInput.ini` 는 Enhanced Input 클래스를 지정하는 두 줄만 남아 있습니다.

| 액션 | 타입 | 키 |
| --- | --- | --- |
| `MoveAction` | Axis2D | WASD, 방향키, `Gamepad_Left2D` |
| `InteractAction` | Boolean | F, 게임패드 A |
| `NavigateUpAction` | Boolean | ↑, 게임패드 D-Pad 위 |
| `NavigateDownAction` | Boolean | ↓, 게임패드 D-Pad 아래 |

`MoveAction` 은 `bConsumeInput = false` 입니다. 방향키를 선택 커서와 나눠 쓰는데,
기본값(true)이면 이동이 먼저 키를 삼켜 커서가 움직이지 않습니다.

키 입력은 X 축으로 들어오므로, 전후 이동은 `UInputModifierSwizzleAxis` 로 Y 축에
옮기고 후진·좌측은 `UInputModifierNegate` 로 뒤집습니다. 매핑 컨텍스트는
`PawnClientRestart()` 에서 우선순위 0 으로 추가됩니다.

## 상호작용

`IInteractable` 인터페이스 (`Interactable.h`) 를 구현한 액터가 대상입니다.

- `ASpriteCharacter` 의 `InteractionSphere` (기본 반경 **60**, `InteractionRadius` 로
  조절) 가 매 틱 겹친 액터 중 **가장 가까운** 대상을 `CurrentInteractable` 로 잡습니다.
- 반경은 **플레이어 중심에서 대상의 충돌까지** 재므로, 캡슐 반지름(34) 이 그 대부분을
  차지합니다. 60 이면 플레이어 표면에서 약 26 떨어졌을 때부터 프롬프트가 뜹니다 —
  거의 맞닿아야 뜨는 거리입니다. 너무 빡빡하면 이 값을 올리세요.
- 프롬프트는 대상 위가 아니라 **선택지 패널과 같은 자리**(대사창 위 오른쪽) 에
  뜹니다. 상호작용할 때마다 시선이 옮겨 다니지 않게 하려는 것입니다.
- **문구는 대상이 정합니다.** `IInteractable::GetInteractPrompt()` 가 돌려주는 값이고,
  비어 있으면 `ADeceiveHUD::InteractPrompt` 로 대체됩니다.

| 대상 | 문구 |
| --- | --- |
| `AInteractableBox` 같은 오브젝트 | 조사한다 |
| `AAreaTransition` (포탈) | 이동한다 |
- F 를 누르면 `IInteractable::Execute_Interact(Target, this)` 가 호출됩니다.

새 상호작용 대상을 만들려면 액터에 `public IInteractable` 을 붙이고
`Interact_Implementation(AActor* InteractingActor)` 를 구현하면 됩니다.
`BlueprintNativeEvent` 라 블루프린트에서 오버라이드할 수도 있습니다.

### 대사창

대사 내용은 `FDialogueData` 구조체 (`DeceiveDialogueTypes.h`) 에 들어 있습니다.

| 필드 | 역할 |
| --- | --- |
| `Speaker` | 창 위에 뜨는 이름. 비우면 생략 |
| `ChoiceId` | 선택 기록용 식별자. 질문마다 고유하게 |
| `Lines` | 도입부. 한 줄에 창 하나 |
| `Question` | 선택지 질문. **비우면 선택 단계 없이 끝남** |
| `ChoiceA` / `ChoiceB` | 각각 `Label` 과 `ResultLines` |

진행 순서는 **도입 → 질문/선택지 → 선택별 결과** 입니다:

1. 범위 안에서 F → 대상의 `Interact()` 가 `StartDialogue(Dialogue)` 를 호출
2. F → `Lines` 를 한 줄씩 진행
3. `Lines` 가 끝나면 질문이 뜨고, **대사창 바로 위 오른쪽에 선택지 패널**이 열림
4. **↑ / ↓** 로 고르고 **F** 로 확정 → 해당 선택의 `ResultLines` 가 재생됨
5. 결과가 끝나면 닫히고 다시 "F" 프롬프트가 뜸

선택 직후 `Question` 을 비우기 때문에, 결과 재생이 끝났을 때 질문이 다시 뜨지
않습니다. 창을 닫고 다시 말을 걸면 대상의 원본 데이터에서 처음부터 시작합니다
(`StartDialogue` 가 복사본을 쓰므로 원본은 변하지 않음).

선택 커서는 `▶` 로 표시되고, 고르지 않은 쪽은 흐린 색으로 그려집니다. 방향키는
이동 키와 겹치지만 대사창이 열려 있는 동안 이동이 막혀 있어 충돌하지 않습니다.

`ASpriteCharacter` 가 진행 상태를 들고 있습니다:

| 함수 | 역할 |
| --- | --- |
| `StartDialogue(FDialogueData)` | 대사창을 염 |
| `IsDialogueActive()` / `IsAwaitingChoice()` | 열려 있는지 / 답을 기다리는지 |
| `GetDialogueSpeaker()` / `GetCurrentDialogueLine()` | HUD 가 읽어감. 질문 중이면 질문을 반환 |
| `GetChoiceLabelA()` / `GetChoiceLabelB()` | 선택지 라벨 |
| `GetSelectedChoiceIndex()` | 지금 커서가 어디 있는지 (0 = 첫째, 1 = 둘째) |

대사창이 열려 있는 동안은 **이동이 막힙니다.** 안 그러면 걸어서 멀어져도 창이 남아
있어 깨져 보입니다. 풀려면 `ASpriteCharacter::Move` 의 `IsDialogueActive()` 체크를
지우면 됩니다.

`ADeceiveHUD` 가 화면 하단에 직접 그립니다. 긴 줄은 `WrapText()` 가 자동으로 접고,
공백이 없는 한국어 구간은 글자 단위로 끊습니다. 모양은 `DialogueWidthFraction`,
`DialogueTextScale`, `DialogueBackgroundColor`, `DialogueSpeakerColor`,
`DialogueHint`, `ChoiceHint`, `ChoiceColor` 로 조절합니다.

### 선택 기록

선택한 답은 `UDeceiveGameInstance` 에 쌓입니다. GameInstance 라서 레벨이 바뀌거나
폰이 교체돼도 살아남습니다 — 주인공 3명 전환이 들어와도 그대로 씁니다.
`Config/DefaultEngine.ini` 의 `GameInstanceClass` 로 지정돼 있습니다.

| 함수 | 역할 |
| --- | --- |
| `RecordChoice(FChoiceRecord)` | 저장. 같은 `ChoiceId` 면 기존 기록을 덮어씀 |
| `GetChoiceRecords()` | 전체 기록 |
| `TryGetChoice(ChoiceId, bOutChoseB)` | 그 질문에 답했는지와 무엇을 골랐는지 |
| `ClearChoices()` | 기록과 종료 상태 초기화 |
| `EndGame()` | **종료 처리.** 이때부터 요약 화면이 뜸 |
| `IsGameFinished()` | 종료됐는지 |

기록은 `FChoiceRecord` 로, `ChoiceId` / `Speaker` / `Question` / `ChosenLabel` /
`bChoseB` 를 담습니다. 요약 화면에 그대로 쓰려고 질문과 라벨 텍스트까지 보관합니다.

**요약은 게임 중에는 절대 뜨지 않습니다.** `EndGame()` 이 불린 뒤에만 `ADeceiveHUD`
가 전체 화면 목록을 그리고, 그 시점부터 이동과 상호작용도 막힙니다.

아직 엔딩이 없으므로 `EndGame()` 을 부르는 곳도 없습니다. 나중에 엔딩을 붙일 때
거기서 호출하시면 됩니다. 테스트용으로는 콘솔(`~`)에 `EndGame` 을 쳐도 됩니다.

분기에 쓰려면:

```cpp
bool bChoseB = false;
if (GetGameInstance<UDeceiveGameInstance>()->TryGetChoice(TEXT("WoodenBox_Lid"), bChoseB))
{
    // 답한 적 있음. bChoseB 가 무엇을 골랐는지
}
```

### 테스트용 박스

`AInteractableBox` 가 교차점 `(0, 0, 75)` 에 스폰되고, 플레이어는 `(-500, 0, 120)` 에서
시작합니다 — 상호작용 범위 밖이라 걸어가야 프롬프트가 뜹니다. F 를 누르면
`LogDeceive` 로 로그가 찍히고, 동시에 대사창이 열립니다:

```
LogDeceive: Interact: SpriteCharacter_0 interacted with InteractableBox_0
```

대사는 `Dialogue` (`FDialogueData`) 에 들어 있고, 생성자에 도입 2줄 + 질문 + 선택지
2개 + 각 결과 2줄이 샘플로 박혀 있습니다. 디테일 패널에서 바꾸거나,
`AInteractableBox` 상속 블루프린트를 만들어 지정하면 됩니다.

## 영역과 이동

맵은 `ADeceiveArea` 를 상속한 액터가 절차적으로 만듭니다. **한 번에 하나만 존재**하며,
이동은 현재 영역을 파괴하고 목표 영역을 새로 만드는 방식입니다.

| 영역 | 클래스 | 내용 |
| --- | --- | --- |
| `Alley` | `ACrossAlley` | 십자 골목길. 시작 지점 |
| `Building` | `ARoomArea` | 건물 내부 |
| `NextStreet` | `ARoomArea` | 맵 경계 너머. 빈 방 플레이스홀더. 건물과 구분되도록 넓고 얕게(2000×500) 만들어짐 |

방향은 화면 기준입니다: 화면 위 = 월드 +X, 화면 왼쪽 = 월드 -Y.

출입구는 두 곳이고 둘 다 **-Y 팔(화면 왼쪽 골목)** 에 있습니다:

| 위치 | 가는 곳 |
| --- | --- |
| -Y 팔의 +X 쪽 벽에 뚫린 문 (`DoorCenterY`, 기본 -700) | 건물 내부 |
| -Y 팔 끝 | 다음 거리 |

`AAreaTransition` 이 포탈입니다. **그 안에 서 있는 것만으로는 아무 일도 일어나지
않습니다.** 상호작용 대상(`IInteractable`)이라 "F" 프롬프트가 뜨고, F 를 눌러 질문에
답해야 이동합니다.

| 목적지 | 질문 | 선택지 |
| --- | --- | --- |
| 건물 | 건물에 들어갈까? | 들어간다 / 안 들어간다 |
| 다음 거리 | 다음 장소로 이동할까? | 이동한다 / 남아있는다 |
| 골목길 (방에서 나갈 때) | 밖으로 나갈까? | 나간다 / 남아있는다 |

**첫 번째 선택지가 항상 이동**, 두 번째가 남는 쪽입니다. 문구는 `AAreaTransition` 의
`Dialogue` (`FDialogueData`) 에 들어 있고, 비워두면 목적지에 맞춰 자동으로 채워집니다.

이동은 **다음 틱에** 일어납니다 — `TravelTo` 가 포탈 자신을 파괴하므로 호출 스택을
빠져나온 뒤에 실행해야 합니다.

문구멍은 지오메트리에 실제로 뚫려 있어서, 그냥 두면 걸어서 통과해 맵 밖으로 나가게
됩니다. 그래서 **구멍마다 투명벽**(`LayOutBlocker`) 을 채워 둡니다 — 충돌만 있고
렌더링되지 않습니다. 포탈은 걸어서가 아니라 상호작용으로 지나가므로 문제없습니다.

| 트리거 | 범위 (기본값 기준) |
| --- | --- |
| 건물 문 | 벽 구멍과 **세 축 모두 동일**. X 200~300 (벽 두께), Y -850~-550 (문 폭), Z 0~600 (벽 높이) |
| 맵 경계 | -Y 팔 끝벽에 밀착. X -200~200 (복도 폭), Y -1200~-1100, Z 0~600 |

맵 경계 트리거는 끝막이 벽 안쪽 면(Y=-1200)에 딱 붙어 있어서, 문(Y=-850)과 250만큼
떨어져 있습니다. 둘을 따로 테스트하기 쉽도록 한 것입니다.

스폰될 때마다 위치와 크기가 `LogDeceive` 로 찍히므로, 출력 로그에서 바로 확인할 수
있습니다.

### 포탈 눈으로 보기

트리거는 렌더링되지 않아 투명합니다. 그래서 `AAreaTransition` 이 자기 볼륨을
**목적지 색으로 디버그 박스**로 그립니다 (기본 켜짐, Shipping 빌드에서는 자동 제외).

| 색 | 목적지 |
| --- | --- |
| 초록 | 건물 |
| 하늘색 | 다음 거리 |
| 노랑 | 골목길 (방에서 나가는 포탈) |

벽 안에 들어 있어도 보이도록 **전경에 그립니다**. 끄려면 `AreaTransition.h` 의
`bShowDebugVolume` 기본값을 `false` 로, 선 굵기는 `DebugLineThickness` 로 바꿉니다.

### 스폰된 액터 이름

런타임에 생성되는 액터는 아웃라이너에서 `AreaTransition_0` 같은 자동 번호로 보이므로,
**에디터 라벨**을 따로 붙여 둡니다 (에디터 전용, 게임 동작에는 영향 없음).

| 액터 | 라벨 |
| --- | --- |
| 건물 문 포탈 | `Portal_ToBuilding` |
| 맵 경계 포탈 | `Portal_ToNextStreet` |
| 방 → 골목길 복귀 포탈 | `Portal_ToAlley` |
| 테스트 상자 | `Item_WoodenBox` |
| 영역 | `Area_Alley` / `Area_Building` / `Area_NextStreet` |

로그도 번호 대신 이름으로 찍힙니다:

```
Portal_ToBuilding spawned at X=250 Y=-700 Z=300, half size X=50 Y=150 Z=300, entry 'Door'
Portal accepted: heading to NextStreet, entry 'Door'
Travelled to Building, entry 'Door', placed at X=0 Y=0 Z=120
```

도착 지점은 각 영역의 `GetEntryLocation(EntryName)` 이 정합니다. 골목길은
`FromBuilding`(문 앞), `FromWest`(-Y 팔 안쪽), 그 외(시작 지점)를 압니다.

도착 지점이 포탈 안이어도 문제없습니다 — 겹침으로는 이동하지 않으므로 "F" 프롬프트만
뜹니다.

### 영역 추가하기

1. `EDeceiveArea` 에 값 추가
2. `ADeceiveArea` 상속 액터 작성 (`CreateBoxes` 로 박스 개수 확보 →
   `OnConstruction` 에서 `LayOutBox`, `SpawnContents` 에서 출입구 스폰)
3. `ADeceiveGameMode::SpawnArea` 에 분기 추가

## 카메라

`ASpriteCharacter` 의 스프링암은 Yaw 0 고정(월드 -X 뒤에서 +X 를 바라봄)입니다.

- `TargetArmLength` = 2700 (거리)
- 스프링암 Pitch = -25° (각도)
- `Camera->FieldOfView` = 40

카메라 Yaw 가 고정이므로 시야를 가리는 벽은 -X 쪽으로 정해집니다. `ACrossAlley` 의
`bHideCameraSideWalls` (기본 켜짐) 가 그 벽들을 `SetHiddenInGame` 으로 렌더링만 끕니다 —
충돌은 남아 있어서 이동은 그대로 막힙니다. 에디터 뷰포트에서는 편집하기 쉽도록
계속 보입니다.

숨겨지는 박스는 5개입니다: -X 갈래의 양옆 벽 2개, 남/북 갈래의 카메라 쪽 벽 2개,
-X 끝막이 1개.

## 골목길 치수

`ACrossAlley` 의 `CorridorWidth`, `ArmLength`, `WallHeight`, `WallThickness`,
`FloorThickness` 로 조절합니다. 박스 14개(바닥 2 + 벽 8 + 끝막이 4)로 구성됩니다.

## 아직 없는 것 (요청 범위 밖)

- 스프라이트 애니메이션 / 이동 방향에 따른 좌우 반전
- HD-2D 룩 (틸트시프트 피사계심도, 포스트프로세스, 라이팅)
- 3주인공 스토리 구조, 대화 시스템
