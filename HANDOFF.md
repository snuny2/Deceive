# Deceive — 현재 상태 (2026-10-28)

HD-2D 스토리텔링 게임. 주인공 3명, 각각의 스토리.
**이 문서는 누적 기록이 아니라 지금 상태 그대로입니다.** 세부 사용법은
[README.md](README.md), 무엇을 어디서 고치는지는 [EDITING.md](EDITING.md).

지금까지 만든 것: 돌아다닐 수 있는 맵 3개, 상호작용, 선택지 있는 대사, 선택 기록.
**스토리는 아직 한 글자도 정해지지 않았습니다.**

---

## 1. 환경

| 항목 | 값 |
| --- | --- |
| 엔진 | **UE 5.7** |
| 프로젝트 | `C:\Users\efzxc\Desktop\Deceive` |
| 모듈 | `Deceive` — C++ 소스 28개 파일, 약 2,670줄 |
| 플러그인 | Paper2D (머티리얼 애셋만 사용), EnhancedInput |
| 콘텐츠 애셋 | **0개** (데이터는 `Content/Data/*.json`) |

> 처음엔 UE 5.6으로 요청받았으나 설치된 엔진이 5.7뿐이라 5.7로 맞췄습니다.

---

## 2. 이 프로젝트를 지배하는 제약

Claude Code는 텍스트 파일만 만들 수 있어서 **언리얼 바이너리 애셋을 만들 수 없습니다**
(`.umap`, `.uasset` — 레벨, 머티리얼, 텍스처, 블루프린트, UMG 위젯, Enhanced Input 애셋).

그래서 전부 우회했고, 이것이 아래 모든 설계 결정의 이유입니다:

| 원래 필요한 것 | 대신 쓴 방법 |
| --- | --- |
| 레벨에 배치한 지오메트리 | 박스 메시를 C++에서 절차적으로 조립 |
| 레벨에 배치한 PlayerStart | GameMode 가 런타임 생성 |
| 커스텀 머티리얼 | Paper2D 내장 머티리얼 + `UMaterialInstanceDynamic` |
| `InputAction` / `InputMappingContext` | `NewObject` 로 런타임 생성 |
| UMG 대사창·프롬프트 | `AHUD::DrawHUD()` 에서 캔버스에 직접 그림 |
| DataTable 애셋 | `Content/Data/*.json` 을 직접 파싱 |

**결과: 빈 레벨 하나만 있으면 플레이됩니다.** 레벨에 아무것도 배치할 필요가 없습니다.

---

## 3. 구성

### 영역 (맵)

`ADeceiveArea` 를 상속한 액터가 자기 지오메트리와 내용물을 전부 들고 있습니다.
**한 번에 하나만 존재**하며, 이동은 현재 영역 파괴 + 목표 영역 스폰 + 플레이어 배치입니다.

| 영역 | 클래스 | 내용 |
| --- | --- | --- |
| `Alley` | `ACrossAlley` | 십자 골목길. 시작 지점. 박스 16개 |
| `Building` | `ARoomArea` | 건물 내부. 900 × 700. 박스 7개 |
| `NextStreet` | `ARoomArea` | 맵 경계 너머. 2000 × 500. **빈 방 플레이스홀더** |

방향 규약: **화면 위 = 월드 +X, 화면 왼쪽 = 월드 -Y.**

### 포탈

출입구 2개는 둘 다 -Y 팔(화면 왼쪽 골목)에 있습니다.

**겹친다고 이동하지 않습니다.** 포탈은 상호작용 대상(`IInteractable`)이고, 범위 안에서
**F 를 누르면 바로 이동**합니다. 프롬프트가 이미 "이동한다"라고 말하므로 같은 것을
다시 묻는 질문 단계는 두지 않았습니다.

| 포탈 | 위치 |
| --- | --- |
| `Portal_ToBuilding` | -Y 팔의 +X 쪽 벽 구멍 (Y = -700) |
| `Portal_ToNextStreet` | -Y 팔 끝벽에 밀착 |
| `Portal_ToAlley` | 방의 출입구 |

**세 가지 주의할 점:**

1. 문구멍은 지오메트리에 **실제로 뚫려 있습니다.** 그냥 두면 걸어서 통과해 맵 밖으로
   나가므로, 구멍마다 **투명벽**(`LayOutBlocker` — 충돌만 있고 렌더링 없음) 을 채워
   뒀습니다. 골목길 Box15, 방 Box6.
2. 트리거는 투명하므로 **목적지 색 디버그 박스**로 그립니다 (초록 = 건물,
   하늘색 = 다음 거리, 노랑 = 골목길). 벽 안에 있어도 보이도록 전경에 그리고,
   Shipping 빌드에서는 자동 제외됩니다.
3. 이동은 **다음 틱에** 일어납니다 — `TravelTo` 가 포탈 자신을 파괴하므로.

### 캐릭터와 카메라

빌보드 스프라이트(`/Engine/BasicShapes/Plane` 을 세워 Yaw 만 회전).

| 항목 | 값 |
| --- | --- |
| 스프라이트 | 120 × 180 |
| 이동 속도 | 350 |
| 캡슐 반지름 / 반높이 | 34 / 88 |
| 카메라 거리 / 각도 / FOV | 2700 / -25° / 40 |

카메라 Yaw 는 0 고정입니다. **Yaw 를 바꾸면 벽 숨김 규칙과 이동 방향이 둘 다
어긋납니다** — 둘 다 "카메라는 -X 에서 +X 를 본다"를 전제로 합니다.

### 입력 (Enhanced Input, 전부 런타임 생성)

| 액션 | 키 |
| --- | --- |
| 이동 | WASD, 방향키, 게임패드 왼쪽 스틱 |
| 상호작용 / 다음 줄 / 선택 확정 | F, 게임패드 아래 버튼 |
| 선택 커서 이동 | ↑ ↓, D-Pad 위/아래 |

### 상호작용과 프롬프트

`IInteractable` 을 구현한 액터가 대상입니다. `InteractionRadius`(기본 **60**) 구체가
겹친 대상 중 가장 가까운 것을 잡습니다.

반경은 **플레이어 중심에서 대상의 충돌까지** 재므로 캡슐 반지름 34 가 대부분을
차지합니다. 60 이면 플레이어 표면에서 약 26 — 거의 맞닿아야 뜹니다.

프롬프트는 대상 머리 위가 아니라 **선택지 패널과 같은 자리**(대사창 위 오른쪽) 에
뜹니다. 문구는 **대상이 정합니다** (`GetInteractPrompt()`):
오브젝트는 "조사한다", 포탈은 "이동한다".

### 데이터 파일 (`Content/Data`)

대사와 아이템은 JSON 에서 읽습니다. **고쳐도 빌드가 필요 없습니다.**

| 파일 | 내용 |
| --- | --- |
| `Dialogues.json` | 이름 → `FDialogueData` |
| `Items.json` | 이름 → `FItemData` (라벨, 프롬프트, 대사 후보 목록, 크기, 영역, 좌표) |

`UDeceiveDataSubsystem`(GameInstance 서브시스템) 이 인스턴스 생성 시 한 번 읽고,
콘솔 `ReloadData` 로 다시 읽습니다. 아이템은 자기 `area` 가 생길 때
`ADeceiveArea::SpawnContents` 가 스폰하므로, 영역마다 코드를 고칠 필요가 없습니다.

**두 파일 모두 UTF-8 BOM 필수.** BOM 이 없으면 `FFileHelper::LoadFileToString` 이
한글을 깨뜨립니다.

패키징 시 복사되도록 `Config/DefaultGame.ini` 에
`+DirectoriesToAlwaysStageAsNonUFS=(Path="Data")` 를 넣어 뒀습니다 (Content 기준 경로).

### 플래그로 대사 고르기

아이템의 `dialogue` 는 **후보 목록**입니다. 상호작용할 때마다
`UDeceiveDataSubsystem::ResolveDialogue` 가 **조건이 맞는 첫 항목**을 고릅니다 —
조건부를 앞에, 조건 없는 기본을 맨 뒤에.

- 대사 항목의 `condition` = `{flag, value}`. `ConditionFlag` 가 None 이면 항상 통과
- 선택지의 `setFlags` = `{이름: 값}`. 고르는 순간 `SelectChoice` 가 적용

**조건은 상호작용할 때마다 다시 풉니다** — 스폰 시 한 번 굽지 않습니다.
그래서 `AInteractableBox` 는 대사가 아니라 `FItemData` 를 들고 있습니다.

나무 상자: 열면 `Has_RustyKey` / `WoodenBox_Opened` 가 서고, 다시 조사하면
`WoodenBox_Empty` ("빈 상자다.") 가 나옵니다.

포탈 대사만 예외로 코드에 있습니다 — 목적지에서 문구가 자동으로 나오기 때문입니다.

### 대사

`FDialogueData` 한 덩어리입니다: `Speaker`, `ChoiceId`, `Lines`, `Question`,
`ChoiceA`, `ChoiceB`. 진행은 **도입 → 질문/선택지 → 선택별 결과 → 닫힘**.
`Question` 을 비우면 선택 단계 없이 끝납니다. 대사 중에는 이동이 막힙니다.

선택의 결과로 할 수 있는 일은 **플래그를 세우는 것**(`setFlags`) 뿐입니다. 임의의
코드를 실행할 방법은 없습니다 — 포탈이 쓰던 `StartDialogueWithHandler` 가 있었지만
포탈이 질문을 안 하게 되면서 지웠습니다. 다시 필요해지면 되살리면 됩니다.

### 플래그와 선택 기록

`UDeceiveGameInstance` 에 있습니다. 레벨 전환과 폰 교체를 넘어 살아남아야 해서
GameInstance 를 골랐습니다 — 주인공 3명 전환이 들어오면 필수가 됩니다.

**플래그가 기억의 본체입니다.** `TMap<FName, int32>` 로, 답변(0 = 첫 선택지), 주운
물건, 열어 둔 문, 횟수 무엇이든 담습니다. API 는 `SetFlag` / `GetFlag` / `HasFlag` /
`ClearFlag` / `GetFlags`. 선택을 기록하면 `ChoiceId` 가 그대로 플래그 이름이 되고,
`TryGetChoice` 도 플래그를 읽습니다.

`FChoiceRecord` 배열은 **종료 요약 화면용**입니다. 질문과 라벨 텍스트를 들고 있습니다.

**`ChoiceId` 가 비어 있는 질문은 기록도 플래그도 남기지 않습니다.** 포탈 질문이
종료 요약에 쌓이면 노이즈이기 때문입니다.

`EndGame()` 이 불린 뒤에만 전체 화면 요약이 뜹니다. **게임 중에는 절대 안 뜹니다.**
아직 엔딩이 없어 부르는 곳이 없으니, 테스트는 콘솔에 `EndGame`.

### 액터 라벨

런타임 스폰이라 아웃라이너에서 자동 번호로 보이므로 에디터 라벨을 붙여 뒀습니다:
`Portal_ToBuilding`, `Portal_ToNextStreet`, `Portal_ToAlley`, `Item_WoodenBox`,
`Area_Alley` / `Area_Building` / `Area_NextStreet`.

---

## 4. 검증 상태

### 실제로 확인한 것

- 빌드 성공 (에디터·게임 타깃, 프로젝트 코드 경고 0)
- 화면 캡처로 눈으로 확인: 캐릭터 빌보드, 대사창(한글 정상), 선택지 패널과 커서 이동,
  선택별 다른 결과, 종료 요약, 건물 내부, 다음 거리, 포탈 디버그 박스,
  프롬프트 문구("조사한다" / "이동한다") 와 선택지 패널의 정렬
- 키를 보내 끝까지 확인: F → `Interact()` → 로그, 선택 확정 → `Choice recorded`,
  `EndGame` → 요약 표시, 포탈 상호작용 → 선택 → `Travelled to ...`
- 포탈 치수를 로그와 수식 계산으로 교차 확인 (구멍 vs 트리거 세 축 일치),
  투명벽 두 개의 위치·크기·`bHiddenInGame`
- **조건부 대사와 플래그**: 첫 조사 → `Interact: ... says 'WoodenBox'`,
  "열어본다" → `Flag set: Has_RustyKey = 1` / `WoodenBox_Opened = 1`,
  다시 조사 → `says 'WoodenBox_Empty'` 와 화면의 "빈 상자다."
- **JSON 전환 후 동작 동일성**: `Data loaded: 1 dialogue(s), 1 item(s).`,
  `Item spawned: WoodenBox at X=0 Y=0 Z=75`(이전 하드코딩 위치와 같음),
  화면의 화자·도입·질문·선택지·결과 문구가 전부 이전과 동일,
  `Flag set: WoodenBox_Lid = 0` → `Choice recorded: WoodenBox_Lid = 열어본다`
- **사용자 확인**: 벽에 막히는 것, 맵 밖으로 나갈 구멍이 더 없는 것,
  상호작용 거리가 적당한 것

### 확인 못 한 것

- 포탈에서 **"남아있는다" 를 골랐을 때 아무 일도 안 하는 것** — 코드로만 확인
  (한 줄 분기)
- 플래그의 `GetFlag` / `HasFlag` / `ClearFlag` — `SetFlag` 만 로그로 확인했고,
  나머지는 `TryGetChoice` 를 통해 간접적으로만 쓰임
- **패키징한 빌드에서 JSON 이 실제로 복사되는지** — `DefaultGame.ini` 설정은 넣었지만
  패키징을 돌려보지는 않았음
- 에디터 뷰포트에서의 `OnConstruction` 실시간 반영
- 실제 텍스처를 넣었을 때의 모양 (플레이스홀더로만 확인)

---

## 5. 개발·테스트 함정 (전부 실제로 겪은 것)

### 빌드

- **에디터가 켜져 있으면 Live Coding 이 빌드를 막습니다.** 종료 후 빌드.
  헤더를 고치면 핫리로드 불가, 에디터 재시작 필수.
- 코드 변경 후 첫 실행은 로딩이 **70초 이상** 걸립니다.
- 헤더 이름이 엔진과 겹치면 UHT 가 막습니다 (`DialogueTypes.h` → `DeceiveDialogueTypes.h`).
- 생성자에서 애셋을 불러올 땐 `LoadObject` 말고 `ConstructorHelpers::FObjectFinder`.

### Enhanced Input

- **`UInputAction::bConsumeInput` 기본값이 true.** 이동과 UI 가 같은 키를 공유하면
  이동이 먼저 삼켜서 UI 가 안 먹습니다. `MoveAction->bConsumeInput = false` 로 해결.
  이동 키와 UI 키를 공유할 때마다 재발하는 문제입니다.

### 파괴된 액터를 한 프레임 더 들고 있음

포탈을 쓰면 `TravelTo` 가 영역과 그 포탈들을 파괴하는데, **플레이어가 들고 있던
`CurrentInteractable` 은 그 프레임 동안 죽은 액터를 계속 가리킵니다.** 그 상태로
`Execute_GetInteractPrompt` 를 부르면 빈 FText 가 돌아오고, HUD 가 대체값
("조사한다") 을 그려서 포탈을 쓸 때마다 엉뚱한 문구가 한 프레임 번쩍였습니다.

`GetCurrentInteractable()` 이 `IsValid` 로 걸러서 해결했습니다. 겹침 목록도 같은
프레임에 파괴된 액터를 담을 수 있으므로 후보 루프에서도 거릅니다.

> 이 증상은 **소환(`summon`)한 포탈로는 재현되지 않습니다.** 영역이 소유하지 않아
> 파괴되지 않기 때문입니다. 영역이 스폰한 진짜 포탈이어야 합니다.

### 지오메트리

- `FVector Extent` 는 **절반 크기**입니다. 이것 때문에 포탈 Z 를 한 번 틀렸습니다.
  로그에도 `half size` 라고 박아 뒀습니다.
- 문구멍은 **진짜 구멍**입니다. 포탈을 걸어서 지나가지 않게 바꾼 뒤로는 반드시
  투명벽을 같이 넣어야 합니다.

### 테스트 자동화

- `SendKeys` 는 `{UP}` 을 전달하지 못합니다(`{DOWN}` 은 됨). 방향키와 콘솔 키
  (VK_OEM_3) 는 `keybd_event` 로 가상 키코드를 직접 보내야 확실합니다.
- `SetForegroundWindow` / `AppActivate` 가 자주 실패해 키가 다른 창으로 샙니다.
  스크린샷에 다른 창이 겹쳐 찍히면 그 실행은 믿으면 안 됩니다.
- `Stop-Process -Force` 로 끄면 **로그 끝부분이 flush 되지 않아 잘립니다.**
  콘솔에 `quit` 을 쳐서 정상 종료해야 전체 로그가 남습니다.
- `HighResShot` 은 `-ExecCmds` 로 시작 프레임에만 찍히므로 입력 후 상태를 못 찍습니다.
  PowerShell 로 창을 직접 캡처(`GetWindowRect` + `CopyFromScreen`)하는 게 확실합니다.
- `-game` 콘솔에는 `sleep` 명령이 없습니다. `DisplayAll` 은 화면 출력이라 nullrhi 에서
  안 잡히므로 `GetAll` 을 씁니다. `GetAll` 의 배열 프로퍼티는 **다음 줄에** 출력되므로
  컨텍스트까지 봐야 합니다 (안 그러면 "비어있다"고 오판합니다 — 실제로 한 번 했습니다).
- `/Engine/Maps/Entry` 에는 엔진 자체 PlayerStart 가 원점에 있어 **시작 위치가 매번
  달라집니다.** 원점에서 시작하면 테스트 상자 위에 스폰되기도 합니다. 걸어서 특정
  지점에 도달하는 테스트가 계속 어긋난 주된 원인이었습니다. 직접 만든 빈 레벨에는
  없는 문제이고, 대신 `summon` 으로 대상을 플레이어 위치에 불러오면 확실합니다.

---

## 6. 아직 없는 것

### 기술

- 스프라이트 애니메이션, 이동 방향에 따른 좌우 반전
- HD-2D 룩: 틸트시프트 피사계심도, 포스트프로세스, 라이팅
- 전환 연출 (페이드 등) — 지금은 한 프레임에 즉시 바뀜
- 엔딩 자체 (`EndGame()` 을 부르는 곳)
- 선택 기록의 디스크 저장 (`USaveGame`) — 지금은 프로세스가 끝나면 사라짐
- 플래그를 **비교 조건**으로만 볼 수 있음 (같다/다르다뿐, 크다·작다·AND·OR 없음)
- 선택의 결과로 임의의 코드를 실행하는 수단 (플래그 설정만 가능)
- `Dialogues.json` 에 조건부 줄 (플래그에 따라 다른 대사) 을 쓸 방법
- 선택지 3개 이상 (지금은 A/B 2개 고정)
- 대사창 크기가 내용에 따라 바뀌는 문제 (사용자가 나중으로 미뤄둠)
- `NextStreet` 는 빈 방일 뿐 실제 거리가 아님
- 텍스처 타일링 (벽 하나에 텍스처 한 장이 늘어남)

### 게임 설계 — **아직 아무것도 정해지지 않음**

- 주인공 3명이 각각 누구인지, 각자의 스토리가 무엇인지
- 세 스토리의 관계: 평행인가, 교차인가, 시간순인가
- 플레이어가 셋 사이를 **어떻게 전환**하는가
- "Deceive" 가 가리키는 것 — 속임수가 메커닉인가, 서사 주제인가
- 십자 골목길이 어떤 장소인가 (허브인가, 특정 챕터 무대인가)
- 대사 분기 데이터 구조, 선택이 결과에 어떻게 반영되는가

> 제목과 주인공 3명 구성을 보면 같은 사건을 세 관점에서 보여주며 서로의 진술이
> 어긋나는 구조로 짐작되지만, **확인된 바 없습니다.**

---

## 7. 실행 방법

1. `Deceive.uproject` 우클릭 → Generate Visual Studio project files
2. 빌드 (Development Editor / Win64), 또는 `.uproject` 더블클릭 후 리빌드에 Yes
3. File → New Level → **Empty Level**, 저장
4. Project Settings → Maps & Modes 에서 Editor/Game Default Map 을 그 레벨로 지정
5. Play — WASD 이동, F 상호작용, ↑↓ 선택, F 확정

빠른 확인:
`UnrealEditor-Cmd.exe Deceive.uproject /Engine/Maps/Entry -game`
(단 위에 적은 PlayerStart 문제가 있습니다)

### 헤드리스 실행이 현재 깨져 있음

`Deceive.exe -game -nullrhi` 가 게임 코드에 도달하기 전에
`FGenerationInfo::Serialize` 어서션으로 죽습니다 (패키지 요약 읽기).
HEAD 코드로 되돌려 빌드해도 똑같이 죽으므로 **코드 문제가 아니라 환경 문제**입니다.
그동안 쓰던 로그 기반 검증은 당분간 안 되고, 에디터에서 직접 확인해야 합니다.

### 에디터에서 테스트할 때는 `DeceiveEditor` 타깃을 빌드해야 함

`Deceive`(게임 실행 파일)만 빌드하면 에디터는 예전 `UnrealEditor-Deceive.dll` 을
계속 읽습니다. 암전 작업에서 이걸로 "코드가 안 먹는다"는 오진을 했습니다.
에디터 확인이 목적이면:

```
Build.bat DeceiveEditor Win64 Development -Project=<...>\Deceive.uproject
```

### 인벤토리

`I` 로 열고 닫습니다. 데이터는 `Content/Data/Inventory.json`,
보유 목록은 `UDeceiveGameInstance::Inventory`(12칸), 화면은
`ADeceiveHUD::DrawInventory`. 아이템은 대사 선택지의 `addItems` 로 들어옵니다.

남은 중복 하나: 나무 상자의 `열어본다` 는 `Has_RustyKey` 플래그와
`addItems: ["RustyKey"]` 를 둘 다 설정합니다. 지금은 플래그 쪽을 아무도 읽지
않으니, 조건 대사에 쓸 생각이 없으면 지워도 됩니다.

### 잠긴 문

`CrossAlley` 오른쪽(+Y) 팔 끝 벽에 구멍을 내고 `AAreaTransition` 을 하나 더
두었습니다. `RequiredItem` 이 비어 있지 않으면 그 아이템을 들고 있을 때만
이동하고, 없으면 `LockedDialogueIds` 로 대사를 띄웁니다 (`AAreaTransition::SayLocked`).
목적지는 새 영역 `EDeceiveArea::LockedRoom` 이고, 돌아오는 입구는 "FromEast" 입니다.

`CrossAlley` 의 박스 개수는 18개입니다 — 칸을 추가/삭제하면
`NumAlleyBoxes` 도 같이 고쳐야 합니다.

### 메뉴 화면과 저장

인벤토리 화면이 **탭 메뉴**가 됐습니다 (정보 / 인벤토리 / 저장 / 설정).
상태는 `ASpriteCharacter` 에 있습니다 — `bMenuOpen`, `ActiveTab`,
`bFocusOnTabs`(커서가 탭 띠에 있는지), `SelectedItemIndex`, `SelectedSaveIndex`.
그리기는 `ADeceiveHUD::DrawMenu` 가 탭 띠를 그리고 탭별 함수로 넘깁니다.

저장은 `UDeceiveSaveGame` + `UGameplayStatics::SaveGameToSlot` 입니다.
칸 목록은 `Deceive_00` ~ `Deceive_19` 를 `DoesSaveGameExist` 로 훑어서 만들고
(별도 인덱스 파일 없음), 메뉴를 열 때와 저장 직후에만 갱신해서 캐시합니다.

아직 안 된 것:
- 정보 / 설정 탭은 빈 자리 표시만
- 저장 삭제, 덮어쓰기 선택, 불러오기 확인 절차 없음 (고르면 바로 불러옴)
- 대사 도중 상태는 저장되지 않음 — 대사 중에는 메뉴가 안 열리므로 문제되지 않음

### 알림 상자

`ASpriteCharacter::ShowNotice(FText)` 가 화면 가운데에 몇 초짜리 문구 상자를
띄웁니다 (`NoticeDuration`, 월드 시간 기준 만료). 그리는 쪽은
`ADeceiveHUD::DrawNotice` 이고, 메뉴와 대사창 **뒤에** 그려서 메뉴를 닫는
불러오기에서도 보입니다. 저장/불러오기 결과를 알리는 데 쓰고 있습니다.

저장 기록은 전용 상자 안에 그리고, 상자에 안 들어가는 줄은 아예 그리지 않습니다
(잘려 보이지 않게). 넘치면 오른쪽에 스크롤바가 생깁니다.

### 저장 목록 스크롤

캔버스에는 클리핑이 없어서 `DrawClippedRect` / `DrawClippedBorder` 로 직접
자릅니다. 목록은 **픽셀 단위**로 스크롤하고, 스크롤량은 선택된 줄이 상자 안에
완전히 들어오도록 매 프레임 계산합니다 (HUD에 상태를 두지 않음).
글자는 반쪽으로 못 그리므로, 그 줄이 상자 안에 완전히 들어왔을 때만 그립니다.

이전 버전은 "통째로 들어가는 줄만 그린다"였는데, 그래서 마지막 칸이 통째로
사라져 저장이 안 된 것처럼 보였습니다.

### 방 모양과 문 위치

`ARoomArea` 가 `DoorSide`(Front / Left / Right)를 받습니다. 벽 네 개 중
해당하는 하나만 문 폭만큼 갈라서 깔고, 막는 벽·복귀 포탈·입장 위치가 모두
`GetDoorwayCenter()` 를 따라갑니다. 박스 개수는 그대로 7개입니다
(갈라진 벽이 1개 대신 2개).

- 왼쪽 거리(NextStreet): 500 × 2600 세로 복도, 문은 Right
- 잠긴 방(LockedRoom): 1600 × 1600 정사각, 문은 Left
- 건물(Building): 900 × 700, 문은 Front (그대로)

람다 인자 이름을 `bHidden` 으로 두면 `AActor::bHidden` 을 가려서 빌드가
실패합니다 (C4458, WarningsAsErrors).

### 소모성 열쇠

`AAreaTransition` 의 잠금 부분이 늘었습니다 — `UnlockDialogueIds`,
`bConsumeRequiredItem`(기본 true), `UnlockedFlag`. 여는 순서는
플래그 세우기 → 아이템 소모 → 여는 대사 → **대사가 닫힐 때 이동** 입니다.

대사가 끝난 뒤에 무언가를 하려고 `ASpriteCharacter::StartDialogueThen(대사,
FSimpleDelegate)` 를 추가했습니다. `CloseDialogue` 에서 핸들러를 먼저 떼어내고
실행하므로, 핸들러가 또 다른 대사를 시작해도 안전합니다.

여는 대사의 `{item}` 은 `UDeceiveDataSubsystem::FindInventoryItem` 으로 찾은
이름으로 치환됩니다.

### 이동 중에는 프롬프트를 숨김

대사창이 닫히고 암전이 시작되기까지 사이에 "문을 연다" 프롬프트가 다시
떴습니다. 암전 첫 프레임은 알파가 0이라 `GetFadeAlpha() > 0` 으로는 못 막습니다.
그래서 `ADeceiveGameMode::IsTravelling()`(페이즈가 None이 아님)을 보고
`DrawInteractPrompt` 에서 빠져나옵니다.

### 옮기는 오브젝트

`FItemData::bCarryable`(Items.json 의 `carryable`) 가 켜진 오브젝트는
`StartDialogueThen` 으로 질문을 띄우고, 대사가 끝나면
`UDeceiveGameInstance::TryGetChoice(ChoiceId)` 로 답을 읽어 A를 골랐으면
`ASpriteCharacter::StartCarrying(this)` 를 부릅니다. 선택지에 새 명령어를
만들지 않고 기존 선택 기록을 그대로 쓴 것입니다 — 그래서 **질문에 `choiceId`
가 없으면 잡히지 않습니다.**

옮기는 동안에는 월드에서 액터를 파괴하고 인벤토리에 아이템으로 넣습니다
(`StartCarrying`). 내려놓을 때 현재 영역의 `SpawnItem` 으로 다시 만듭니다
(`StopCarrying`). 끌고 다니는 방식은 썼다가 버렸습니다 — 내려놓은 물건이
보이지 않는 문제가 있었고, 사용자가 인벤토리 방식을 원했습니다.

아이템이 어디 있는지는 `UDeceiveGameInstance::ItemPlacements`
(`bHeld` / `Area` / `Location`) 가 기억하고, `ADeceiveArea::SpawnContents` 가
이걸 보고 **표의 위치 대신 놓인 자리에** 만듭니다. 들고 있는 것은 아예 안 만듭니다.
저장 파일에도 들어갑니다.

지금은 `LockedRoom` 의 `StoneBlock` 하나가 이걸 씁니다.

### 큰 사고 하나

`SpriteCharacter.cpp` 를 "함수 A부터 함수 B 직전까지 잘라내고 새 내용으로
교체" 하는 스크립트로 고치다가, 그 사이에 있던 함수 20여 개를 통째로 날렸습니다
(링커 오류로 발견). `git show HEAD:<파일>` 의 옛 버전을 기준으로 이번 세션의
변경을 다시 얹어 복구했고, HEAD에 있던 함수가 전부 돌아왔는지 대조했습니다.

**교훈: 범위를 잘라내는 방식 대신 함수 단위로 바꿀 것.** 꼭 잘라내야 한다면
잘라낼 구간을 먼저 출력해서 확인할 것.

### 돌무더기 / 돌맹이 (집기 · 내려놓기)

들고 다니는 구조를 다시 바꿨습니다. 지금은:

- 인벤토리는 **같은 아이템을 중복으로 담습니다** (한 칸에 하나씩).
  `RemoveInventoryItem` 은 `RemoveSingle` 이어야 합니다 — `Remove` 는 같은 값을
  전부 지웁니다.
- 집기는 대사 선택지의 `addItems` 그대로입니다 (새 코드 없음).
- 내려놓기는 인벤토리 탭에서 `F` → `ASpriteCharacter::PlaceSelectedItem()`.
  `Inventory.json` 의 `placeable` 과 `Items.json` 의 같은 id 항목이 둘 다 있어야
  합니다.
- 내려놓은 것은 `UDeceiveGameInstance::PlacedItems`(배열)에 쌓이고,
  `ADeceiveArea::SpawnContents` 가 표 항목 + 이 배열을 둘 다 만듭니다.
  `placedOnly` 항목은 표에서 안 만듭니다.

`AInteractableBox` 의 CarryIfAccepted, `StartCarrying/StopCarrying`,
HUD 의 `CarryPrompt` 는 전부 지웠습니다.

### 돌맹이 — 다시 줍기 · 통과 · 가득 참

- 내려놓은 사본마다 `PlacedId`(1부터, 재사용 안 함)가 붙습니다.
  `AInteractableBox::PlacedId` 가 0이 아니면 내려놓은 사본이고, 그때만
  `TakeIfAccepted` 로 다시 주울 수 있습니다. 주우면
  `RemovePlacedItem(PlacedId)` 로 그 하나만 지웁니다.
- `walkThrough` 가 켜진 아이템은 메시 콜리전이 `OverlapAllDynamic` 이라 길을
  막지 않습니다. 상호작용은 겹침으로 잡히므로 그대로 됩니다.
- 내려놓는 위치는 플레이어 위치 그대로입니다 (`DropDistance` 는 없앴습니다).
- 인벤토리가 가득 차면 `AddInventoryItem` 이 false 를 돌려주고, 대사 `addItems`
  쪽과 다시 줍기 쪽 양쪽에서 알림 상자를 띄웁니다.

### 키 배치 (Z / X / Shift / C)

`EnsureInputObjects()` 에서 바뀝니다 — Z 상호작용, X 취소, Shift 달리기,
C 메뉴. 달리기는 `RunAction` 의 Started/Completed 에서
`MaxWalkSpeed` 를 `WalkSpeed * RunSpeedMultiplier` 로 올렸다 되돌립니다.
X(`Cancel`)는 메뉴에서만 동작하고, 내용 → 탭 → 닫기 순으로 한 단계씩 빠집니다.
대사 중에는 아무 일도 하지 않습니다.

화면 힌트 문구(`DialogueHint`, `ChoiceHint`, `InventoryHint`, `SaveHint`)도
같이 고쳐야 합니다 — 키를 또 바꾸면 여기도 손볼 것.

### 가득 찬 소지품은 대사로

알림 상자로 띄웠더니 선택지 결과 대사("집어 넣었다")와 정반대 내용이 동시에
보였습니다. 지금은:

- 대사 `addItems` 로 들어오다 실패하면 `SelectChoice` 가 결과 대사 대신
  `InventoryFullLine` 을 보여줍니다.
- 내려놓은 물건을 다시 줍는 쪽은 **질문하기 전에** `IsInventoryFull()` 을 보고
  `SayInventoryFull()` 로 끝냅니다 (묻지 않음).

### 메뉴 구역과 선택지 취소

메뉴는 탭 구역 / 내용 구역 두 개로 나뉘고, 방향키는 구역 안에서만 움직입니다
(`MoveMenuCursorSideways` 는 내용 구역의 격자에서만 동작). 구역 이동은
`ActivateMenuRow`(Z, 탭 → 내용)와 `Cancel`(X, 내용 → 탭 → 닫기)뿐입니다.
메뉴는 탭 구역에서 열립니다.

선택지가 떠 있을 때 X 는 `CancelDialogue()` — **먼저 `DialogueClosedHandler`
를 풀고** 닫습니다. 안 그러면 답하지 않았는데도 문이 열리거나 물건을 줍습니다
(그 핸들러들은 선택 기록을 되읽는 구조라, 예전 기록이 남아 있으면 그대로
실행됩니다). 평범한 대사 줄은 X 로 건너뛸 수 없습니다.

### 정보 탭과 플레이어 데이터

`Content/Data/Player.json` → `FDeceivePlayerData`(이름/초상화/최대체력/상태).
`UDeceiveGameInstance::Init()` 이 Super::Init 뒤에 서브시스템에서 읽어
현재 체력과 상태를 채웁니다 (서브시스템은 Super::Init 중에 올라옵니다).
현재 체력·상태는 저장 파일에도 들어갑니다. 체력을 깎는 코드는 아직 없습니다.

그리기는 `ADeceiveHUD::DrawInfoTab` — 초상화, 이름, 상태, 체력 막대.
초상화도 `ResolveItemTexture` 를 그대로 써서 경로가 비면 대체 이미지로 갑니다.

메뉴는 열 때마다 `ActiveTab = Info` 로 되돌립니다 (저장 탭에서 닫고 다시 열면
저장 탭이 나오던 문제).

선택지 취소(X)는 그냥 닫지 않고 `CancelledLine`("다시 생각해보자.") 한 줄을
남깁니다 — 핸들러는 먼저 풀어둡니다.

### 잠긴 방 퍼즐

`ALockedRoomArea : ARoomArea` 가 방 + 철창 + 발판을 담당합니다.

- `ADeceiveArea::CreateBoxes` 가 이름을 `Boxes.Num()` 부터 번호 매기도록
  바꿨습니다. 그래야 자식 클래스가 생성자에서 박스를 더 만들 수 있습니다
  (`Box0` 이름 충돌 방지). 잠긴 방은 7개(방) + 3개(철창 2 + 문 1) = 10개.
- 문은 액터가 아니라 박스 하나입니다. 열릴 때 `SetHiddenInGame(true)` +
  콜리전 해제, 닫힐 때 되돌립니다.
- **문 상태는 발판에서 매 틱 계산합니다** (래치 아님). 발판에서 무게가 빠지면
  닫힙니다. 그래서 플래그로 기억할 것이 없습니다 — 내려놓은 물건 위치가
  저장되므로 다시 들어오면 알아서 같은 상태가 됩니다.
  방에 들어와 처음 한 번은 `bFirstCheck` 로 대사 없이 상태만 맞춥니다.
- 발판(`APressurePlate`)은 판정만 합니다 (`IsPressed()` 가 겹친 액터 중
  Pawn 이나 `AInteractableBox` 가 있는지 확인). 퍼즐 판단은 방이 Tick 에서
  합니다 — 발판이 늘어도 코드는 그대로입니다.
- 마지막 발판이 눌리는 프레임에는 `Plate_Click` 을 건너뛰고 `Gate_Open` 만
  띄웁니다. 안 그러면 같은 프레임에 대사창을 두 번 덮어씁니다.

발판 문구는 **알림 상자**입니다 (`Notice()` — Dialogues.json 항목의 첫 줄만
씁니다). 대사창이면 발판을 밟을 때마다 이동이 멈춰서 바꿨습니다.
문이 열리고 닫히는 것만 대사창(`Say()`)이고, 문 상태가 바뀐 프레임에는
발판 알림을 건너뜁니다 (한 프레임에 둘 다 뜨지 않도록).

### 글자 방 (OPEN 퍼즐)

새 영역 `EDeceiveArea::PuzzleRoom` / `AWordPuzzleRoom`(ADeceiveArea 직접 상속).
골목 -X 팔 끝 → 글자 방 → (풀면) 왼쪽 긴 복도 아래쪽, 이렇게 이어집니다.

- `WordRows` 문자열 격자가 글자 전부입니다. `#` 책상, 숫자는 그 색 빈칸.
  책상 개수를 세어 `CreateBoxes` 에 넘기므로 글자를 바꿔도 박스 수는 자동입니다.
  박스 배치: 0 바닥, 1-3 뒷벽(문 2개로 3토막), 4 앞벽, 5/6 옆벽, 7/8 막는 벽,
  9부터 책상.
- `APushableBox` 는 플레이어 속도를 받아 **한 축으로만** sweep 이동합니다
  (당기기 없음, 책상·벽에 막힘). 색은 `DrawDebugBox`.
- 빈칸은 액터가 아니라 방이 들고 있는 `FWordPuzzleSlot` 배열입니다. 방이 Tick 에서
  색이 같은 상자가 `SnapDistance` 안에 들어왔는지 보고 자리에 앉힙니다.
- 다 채우면 `WordPuzzle_Solved` 플래그 → 나가는 포탈의 `RequiredFlag` 가 그것.

`AAreaTransition` 에 `RequiredFlag` 를 추가했습니다 (아이템이 아니라 플래그로
잠그는 문). 아이템 잠금과 달리 소모도 해제도 없습니다.

`ARoomArea` 가 **두 번째 문**을 가질 수 있게 됐습니다 (`bHasSecondDoor`,
`SecondDoorSide`, `SecondArea`, `SecondEntry`, `SecondEntryName`).
박스는 항상 9개를 만들고, 문이 하나면 남는 2개를 `DisableBox` 합니다.
**그래서 `ALockedRoomArea` 의 박스 번호가 7~9에서 9~11로 밀렸습니다** —
RoomArea 박스 수를 또 바꾸면 여기도 같이 고쳐야 합니다.

아직 못 한 것: 실제로 밀어서 풀어본 검증 (헤드리스가 여전히 안 돌아감).
미는 느낌(속도·접촉 범위)과 난이도는 플레이로 맞춰야 합니다.
