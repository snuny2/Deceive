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

## 2. 대사와 아이템 — `Content/Data/*.json`

**대사와 아이템은 JSON 파일에 있습니다. 고쳐도 빌드가 필요 없습니다.**
저장하고 Play 만 다시 누르면 됩니다 (실행 중이면 콘솔에 `ReloadData`).

두 파일 모두 **UTF-8 BOM** 으로 저장해야 한글이 깨지지 않습니다. 메모장은 "UTF-8
(BOM)", VS Code 는 우하단 인코딩에서 "UTF-8 with BOM" 을 고르세요.

### `Content/Data/Items.json`

```json
{
  "WoodenBox": {
    "label": "Item_WoodenBox",
    "prompt": "조사한다",
    "dialogue": "WoodenBox",
    "size": 150,
    "area": "Alley",
    "location": { "x": 0, "y": 0, "z": 75 }
  }
}
```

| 키 | 역할 |
| --- | --- |
| 바깥 이름 (`"WoodenBox"`) | 아이템 id |
| `label` | 아웃라이너에 뜨는 이름 |
| `prompt` | 상호작용 프롬프트 문구 |
| `dialogue` | `Dialogues.json` 의 어느 항목을 말하는지. 이름 하나 또는 **후보 목록** |
| `size` | 상자 한 변 길이 |
| `area` | `Alley` / `Building` / `NextStreet` — 그 영역이 생길 때 같이 스폰됨 |
| `location` | 영역 기준 좌표 |

**아이템을 추가하려면 이 파일에 항목을 하나 더 쓰면 끝입니다.** 코드는 건드릴 필요
없습니다.

### 상태에 따라 다른 대사

`dialogue` 를 목록으로 주면, 상호작용할 때마다 **조건이 맞는 첫 항목**이 쓰입니다.
조건부를 앞에, 기본을 맨 뒤에 두세요.

```json
"dialogue": [ "WoodenBox_Empty", "WoodenBox" ]
```

나무 상자가 이렇게 되어 있습니다:

1. 처음에는 `WoodenBox_Empty` 의 조건(`WoodenBox_Opened == 1`)이 안 맞으니
   `WoodenBox` 가 나옴
2. "열어본다" 의 `setFlags` 가 `Has_RustyKey` 와 `WoodenBox_Opened` 를 1 로 세움
3. 다시 조사하면 조건이 맞아 `WoodenBox_Empty` ("빈 상자다.") 가 나옴

"그냥 둔다" 는 플래그를 세우지 않으므로, 몇 번을 다시 조사해도 원래 대사가 나옵니다.

### `Content/Data/Dialogues.json`

```json
{
  "WoodenBox": {
    "speaker": "나무 상자",
    "choiceId": "WoodenBox_Lid",
    "lines": ["첫 줄", "둘째 줄"],
    "question": "뚜껑을 열어볼까?",
    "choiceA": { "label": "열어본다", "resultLines": ["..."] },
    "choiceB": { "label": "그냥 둔다", "resultLines": ["..."] }
  }
}
```

| 필드 | 역할 |
| --- | --- |
| `speaker` | 창 위 이름 (금색). **비우면 이름 줄이 사라짐** |
| `choiceId` | 플래그 이름. **질문마다 다르게.** 비우면 기록도 플래그도 남지 않음 |
| `lines` | 도입부. **배열 한 칸 = 창 하나** |
| `question` | 선택지 질문. **비우면 선택 단계 없이 도입만 하고 끝남** |
| `choiceA.label` / `choiceB.label` | 선택지 문구 |
| `choiceA.resultLines` / `choiceB.resultLines` | 선택 후 나올 줄들 |
| `choiceA.setFlags` / `choiceB.setFlags` | 그 선택이 세우는 플래그. `{"이름": 값}` |
| `condition` | `{"flag": "이름", "value": 1}`. **없으면 항상 통과** |

진행: **도입 → 질문/선택지 → 선택별 결과 → 닫힘**

### 자주 하는 수정

**선택지 없는 평범한 대사로:** `question` 을 `""` 로 두면 됩니다. `lines` 만 재생되고 닫힙니다.

**질문부터 바로 시작:** `lines` 를 `[]` 로 두면 열자마자 질문이 뜹니다.

**한쪽 선택은 아무 말 없이 끝내기:** 그 선택의 `resultLines` 를 `[]` 로 두면 즉시 닫힙니다.

**줄 길이:** 알아서 접히므로 길게 써도 됩니다. 다만 한 칸이 너무 길면 창이 세로로
커지니, 두세 줄 분량에서 배열을 나누는 게 보기 좋습니다.

### 주워서 내려놓는 물건 (퍼즐용)

집는 쪽과 내려놓는 쪽이 따로입니다.

**집기** — 그냥 보통 대사 선택지입니다. `addItems` 로 인벤토리에 넣습니다.
같은 물건을 여러 번 집으면 **칸을 하나씩 차지하며 쌓입니다** (최대 12칸).

```json
"StonePile": {
  "speaker": "돌무더기",
  "choiceId": "StonePile_Take",
  "question": "돌맹이를 집을까?",
  "choiceA": { "label": "집는다", "addItems": [ "Stone" ] },
  "choiceB": { "label": "안 집는다" }
}
```

**내려놓기** — 인벤토리 탭에서 물건을 고르고 `Z`. 선 자리에 생기고
그 칸이 빠집니다. 여러 개를 여기저기 놓을 수 있습니다.

그러려면 **양쪽에 항목이 필요합니다**:

```json
// Inventory.json — 들고 있을 때
"Stone": { "name": "돌맹이", "description": "...", "placeable": true }

// Items.json — 내려놓았을 때 월드에 생기는 것
"Stone": {
  "label": "Item_Stone", "prompt": "조사한다", "dialogue": [ "Stone" ],
  "size": 70, "placedOnly": true,
  "area": "LockedRoom", "location": { "x": 0, "y": 0, "z": 35 }
}
```

| 키 | 뜻 |
| --- | --- |
| `placeable` (Inventory.json) | 인벤토리에서 `Z` 로 내려놓을 수 있음 |
| `placedOnly` (Items.json) | 처음부터 월드에 있지 않고, 내려놓았을 때만 생김 |
| `walkThrough` (Items.json) | 길을 막지 않음 — 밟고 지나갈 수 있음 |
| `location.z` | 내려놓았을 때의 높이 (바닥에 놓이는 높이로) |

내려놓으면 **플레이어가 선 자리 그대로**에 생깁니다. 자리는 전부 기억되고
저장에도 들어갑니다 — 다른 방에 갔다 와도 놓아둔 그대로입니다.

**다시 줍기** — 내려놓은 물건의 대사에 `choiceId` 가 있는 질문을 달면,
첫 번째 선택지를 고를 때 그 하나만 사라지고 인벤토리로 돌아옵니다.
표에서 처음부터 놓여 있던 물건은 이렇게 주울 수 없습니다 (내려놓은 사본만).

```json
"Stone": {
  "speaker": "돌맹이",
  "choiceId": "Stone_Take",
  "lines": [ "내려놓은 돌맹이다." ],
  "question": "다시 주울까?",
  "choiceA": { "label": "줍는다", "resultLines": [ "돌맹이를 다시 집어 들었다." ] },
  "choiceB": { "label": "그냥 둔다", "resultLines": [ "그대로 두기로 했다." ] }
}
```

소지품이 가득 차면 **대사창에** "더는 들 수 없다. 소지품이 가득 찼다." 가
나오고 줍지 못합니다. 선택지의 결과 대사 대신 이 줄이 나오므로, 집었다는 말과
어긋나지 않습니다. 문구는 `SpriteCharacter → Inventory Full Line`.

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

## 3-3. 메뉴 화면 (인벤토리 · 저장)

`I` 로만 열고 닫습니다. 대사 중에는 열리지 않습니다.
맨 왼쪽에 **정보 / 인벤토리 / 저장 / 설정** 탭이 세로로 있습니다.

| 키 | 동작 |
| --- | --- |
화면은 **왼쪽 탭 구역**과 **오른쪽 내용 구역**으로 나뉘고, 방향키는 지금 있는
구역 안에서만 움직입니다. 구역을 넘나드는 건 Z / X 뿐입니다.

| 키 | 동작 |
| --- | --- |
| ↑ ↓ ← → | 지금 구역 안에서 이동 (탭 구역에선 ↑↓ 로 탭 바꾸기) |
| Z | 탭 구역: 내용으로 들어감 / 내용 안: 내려놓기·저장·불러오기 |
| X | 내용 → 탭 구역 → 메뉴 닫기 |
| C | 바로 닫기 |

메뉴를 열면 **탭 구역**에서 시작합니다.

메뉴는 **항상 정보 탭, 탭 구역**에서 열립니다.
**설정 탭은 아직 내용이 없습니다** (문구는 `DeceiveHUD → Menu Placeholder Text`).

### 정보 탭 — `Content/Data/Player.json`

```json
{
  "name": "이름 없음",
  "portrait": "",
  "maxHealth": 20,
  "status": "멀쩡하다."
}
```

| 키 | 뜻 |
| --- | --- |
| `name` | 이름 칸 |
| `portrait` | 사진 애셋 경로. **비우면 대체 이미지** |
| `maxHealth` | 체력 최대치 (게임 시작 시 이 값으로 참) |
| `status` | 상태 칸의 기본 문구 |

지금 체력과 상태는 `UDeceiveGameInstance` 가 들고 있고 저장에도 들어갑니다 —
코드에서 `SetHealth(n)` / `SetStatus(텍스트)` 로 바꿉니다. 아직 체력이 깎이는
곳은 없습니다.

사진·이름·상태·체력은 **하나의 상자 안**에 들어갑니다. 색과 문구는
`DeceiveHUD → Health Color` / `Health Label`(기본 "체력") /
`Status Label`(기본 "상태" — 화면엔 `상태: 멀쩡하다.` 처럼 나옵니다).

### 인벤토리 탭

왼쪽은 **6 × 2 = 12칸 격자**(빈 칸도 그려집니다), 오른쪽은 고른 칸의
**확대 이미지 · 이름 칸 · 설명 상자** 입니다.

### 아이템 등록 — `Content/Data/Inventory.json`

```json
{
  "RustyKey": {
    "name": "녹슨 열쇠",
    "description": "나무 상자 안에 들어 있던 작은 열쇠다.",
    "texture": ""
  }
}
```

| 키 | 뜻 |
| --- | --- |
| `name` | 목록과 오른쪽에 쓰이는 이름 |
| `description` | 오른쪽 설명문. 폭에 맞춰 자동 줄바꿈 |
| `texture` | 이미지 애셋 경로 (예: `/Game/Art/RustyKey.RustyKey`). **비워 두면 대체 이미지** |

이미지를 넣을 때는 텍스처를 임포트하고 그 경로를 `texture` 에 적으면 됩니다.
경로가 틀리면 대체 이미지로 떨어지고 로그에 경고가 남습니다.
대체 이미지 자체를 바꾸려면 `DeceiveHUD → Inventory Placeholder Texture Path`.

### 아이템 주기

대사 선택지에 `addItems` 를 적으면 그 답을 고를 때 들어옵니다.

```json
"choiceA": {
  "label": "열어본다",
  "setFlags": { "WoodenBox_Opened": 1 },
  "addItems": [ "RustyKey" ]
}
```

같은 아이템은 중복으로 안 들어가고, 12칸이 다 차면 들어오지 않고 로그에
경고가 남습니다. 칸 수는 `DeceiveGameInstance.h` 의 `InventoryCapacity` 입니다
(코드 수정 + 빌드 필요)

### 화면 모양 — `DeceiveHUD`

| 프로퍼티 | 역할 |
| --- | --- |
| `Inventory No Selection Text` | 빈 칸을 골랐을 때 설명 상자에 쓰는 문구 |
| `Inventory Hint` | 우하단 조작 안내 |
| `Inventory Slot Color` | 칸과 오른쪽 상자들의 안쪽 색 |
| `Inventory Border Color` | 고르지 않은 칸의 테두리 색 (고른 칸은 `Choice Color`) |

칸 수를 바꾸려면 `DeceiveGameInstance.h` 의 `InventoryCapacity` 와
`InventoryColumns`(한 줄에 몇 칸) 를 고치고 빌드하면 됩니다.

---

## 3-4. 저장

저장 탭의 맨 윗줄이 **현재 진행사항 저장**이고, 그 아래 **상자 안에** 저장한
기록이 쌓입니다. 상자보다 많아지면 커서를 따라 픽셀 단위로 스크롤되고, 위아래 칸은 웹처럼
잘려 보입니다. 오른쪽에 스크롤바가 나옵니다. 기록을 고르고 `F` 를 누르면 **바로 불러옵니다** (확인 절차 없음).

저장하거나 불러오면 화면 가운데에 **"저장되었습니다." / "불러왔습니다."** 상자가
잠깐 떴다 사라집니다. 길이는 `SpriteCharacter → Notice Duration` (기본 2.5초).

저장되는 것: 플래그 전부, 인벤토리, 선택 기록, 현재 영역, 플레이어 위치.
파일은 `Saved/SaveGames/Deceive_00.sav` 같은 이름으로 들어갑니다.

| 바꾸고 싶은 것 | 어디서 |
| --- | --- |
| 저장 칸 최대 개수 | `DeceiveGameInstance.h` 의 `MaxSaveSlots` (기본 20, 빌드 필요) |
| 윗줄 문구 | `DeceiveHUD → Save Now Text` |
| 기록 없을 때 문구 | `DeceiveHUD → Save Empty Text` |
| 윗줄 / 기록 테두리 색 | `Save Now Color` / `Save Record Color` |

저장 내용을 늘리려면 `DeceiveSaveGame.h` 에 필드를 추가하고
`UDeceiveGameInstance::SaveToSlot` 과 `LoadFromSlot` 양쪽에 넣으면 됩니다.
**기존 저장 파일은 추가한 필드가 비어 있는 채로 읽힙니다.**

---

## 3-2. 맵 영역과 출입구

한 번에 한 영역만 존재합니다 (`Alley` / `Building` / `NextStreet` / `LockedRoom` /
`PuzzleRoom`).
방향은 화면 기준으로 **위 = +X, 왼쪽 = -Y** 입니다.

| 바꾸고 싶은 것 | 어디서 |
| --- | --- |
| 포탈 프롬프트 문구 | `AreaTransition → Interact Prompt` (기본 "이동한다") |
| 잠긴 문이 요구하는 아이템 | `AreaTransition → Required Item` (비우면 안 잠김) |
| 잠겼을 때 나오는 대사 | `AreaTransition → Locked Dialogue Ids` (Dialogues.json 항목) |
| 건물 문 위치 | `CrossAlley → Door Center Y` (기본 -700, -Y 팔 위에서의 위치) |
| 건물 문 폭 | `CrossAlley → Door Width` (기본 300). 트리거 크기도 같이 따라감 |
| 왼쪽 거리 크기 | `DeceiveGameMode → Next Street Width / Depth` (기본 500 × 2600 — 세로로 긴 복도) |
| 잠긴 방 크기 | `DeceiveGameMode → Locked Room Width / Depth` (기본 1600 × 1600) |
| 암전 길이 | `DeceiveGameMode → Fade Duration` (기본 0.35초. 검어지는 데 그만큼, 밝아지는 데 또 그만큼. 0 이면 암전 없이 즉시 이동) |
| 방 크기 | `RoomArea → Room Width` (가로) / `Room Depth` (깊이) |
| 방 문 폭 | `RoomArea → Door Width` |
| 방 텍스처 | `RoomArea → Floor Texture` / `Wall Texture` |

`RoomArea` 의 문 위치는 `Door Side` 로 정합니다 — `Front`(카메라 쪽 벽),
`Left`(화면 왼쪽), `Right`(화면 오른쪽). 카메라 쪽 벽은 어느 쪽에 문이 있든
렌더링이 꺼져 있어 방 안이 보입니다.

- 건물: `Front` (900 × 700)
- 왼쪽 거리: `Right` — 골목 쪽을 보는 면에 문
- 잠긴 방: `Left` — 골목 쪽을 보는 면에 문

방에 들어가면 **문 바로 안쪽**에 서게 됩니다 (긴 복도에서 한가운데 떨어지지
않도록).

### 잠긴 문

오른쪽(+Y) 팔 끝 벽에 **녹슨 열쇠가 있어야 열리는 철문**이 있습니다.
열쇠가 없으면 `LockedDoor` 대사가 나오고 이동하지 않습니다. 열쇠가 있으면
`LockedDoor_Unlock` 대사가 나오고, **대사를 다 읽은 뒤에** 넘어갑니다.

열쇠는 **쓰면 사라집니다**. 대신 자물쇠가 풀린 사실이 플래그로 남아서,
다음부터는 열쇠 없이 그냥 지나갑니다 (안 그러면 돌아왔을 때 영영 못 들어감).

```cpp
LockedDoor->RequiredItem = TEXT("RustyKey");
LockedDoor->LockedDialogueIds.Add(TEXT("LockedDoor"));
LockedDoor->UnlockDialogueIds.Add(TEXT("LockedDoor_Unlock"));
LockedDoor->UnlockedFlag = TEXT("LockedDoor_Unlocked");
```

| 프로퍼티 | 역할 |
| --- | --- |
| `Required Item` | 있어야 열리는 아이템 (비우면 안 잠김) |
| `Locked Dialogue Ids` | 아이템이 없을 때 나오는 대사 |
| `Unlock Dialogue Ids` | 열 때 나오는 대사 |
| `Consume Required Item` | 켜면 아이템이 사라짐 (기본 켜짐) |
| `Unlocked Flag` | 한 번 열리면 계속 열린 상태로 두는 플래그 |

여는 대사의 `{item}` 은 **쓴 아이템 이름으로 바뀝니다**. 그래서 Inventory.json 의
`name` 만 고치면 대사도 따라갑니다.

```json
"LockedDoor_Unlock": {
  "speaker": "철문",
  "lines": [ "{item}를 꽂아 돌리자, 걸쇠가 무겁게 풀렸다." ]
}
```

> 조사가 붙는 자리라 아이템 이름의 받침에 따라 "열쇠를 / 문서을" 처럼
> 어색해질 수 있습니다. 받침 없는 이름을 쓰거나 문장을 바꿔 쓰면 됩니다.

포탈은 투명하지만 **목적지 색 디버그 박스로 그려집니다** — 초록 = 건물,
하늘색 = 다음 거리, 자홍 = 잠긴 방, 노랑 = 골목길. 벽 안에 있어도 보입니다.
끄려면 `AreaTransition.h` 의 `bShowDebugVolume` 를 `false` 로.

새 영역을 추가하려면 코드가 필요합니다 — README 의 "영역 추가하기" 참고.

## 3-5. 잠긴 방의 퍼즐 — `LockedRoomArea`

방 오른쪽을 **철창**이 가로막고, 그 안쪽에 보상(낡은 함)이 있습니다.
철창 문은 **발판 2개를 동시에 밟아야** 열립니다. 몸은 하나뿐이니 한쪽에는
물건을 올려놔야 합니다 (지금은 돌맹이).

발판은 **플레이어나 내려놓은 물건**이 올라오면 눌립니다. 눌릴 때
`Plate_Click`, 올라올 때 `Plate_Release` 가 **알림 상자**로 뜹니다
(이동이 멈추지 않게). 문이 열리거나 닫힐 때만 **대사창**이 나옵니다.

**문은 두 발판이 눌려 있는 동안에만 열려 있습니다.** 무게가 빠지면 다시
닫히고 `Gate_Close` 대사가 나옵니다. 그래서 직접 밟아서는 통과할 수 없고,
**두 발판 모두에 물건을 올려놔야** 합니다 (돌맹이 2개).

내려놓은 물건의 자리는 저장되므로, 방을 나갔다 와도 발판이 눌린 상태면
문은 그대로 열려 있습니다 (들어올 때 대사 없이 조용히 맞춰집니다).

| 바꾸고 싶은 것 | 어디서 |
| --- | --- |
| 철창 위치 | `LockedRoomArea → Bar Inset` (오른쪽 벽에서 떨어진 거리, 기본 300) |
| 문 위치·크기 | `Gate Center X` / `Gate Width` |
| 발판 위치 | `First Plate` / `Second Plate` (X, Y) |
| 발판 크기 | `PressurePlate → Plate Size` |
| 발판 문구 | `Plate Dialogue Id` / `Plate Released Dialogue Id` (알림, 첫 줄만 사용) |
| 문 대사 | `Gate Dialogue Id` / `Gate Closed Dialogue Id` (대사창) |

**발판을 늘리려면** `SpawnContents()` 에서 발판을 더 스폰하면 됩니다. "전부
눌렸는가" 로 판정하므로 개수는 상관없습니다.

> 아이템별로 다르게 반응하게 하려면 `APressurePlate::IsPressed()` 를 고치면
> 됩니다. 지금은 "플레이어나 아무 물건"이면 눌립니다.

보상은 `Items.json` 의 `RewardChest` 와 `Inventory.json` 의 `Trinket` 입니다 —
이름과 대사는 자리만 잡아둔 것이니 JSON에서 고치시면 됩니다.

---

## 3-6. 글자 방 — `WordPuzzleRoom`

골목 **아래쪽(-X) 팔 끝**의 문으로 들어갑니다. 그 문은 **잠긴 방에서 얻은
장식품(`Trinket`)** 이 있어야 열리고, 장식품은 **없어지지 않습니다**.

방 바닥에 책상으로 **OPEN** 이 쓰여 있고 세 칸이 비어 있습니다. 색이 칠해진
상자를 **밀어서** 같은 색 빈칸에 넣으면 글자가 완성되고, 왼쪽 위 문이 열려
**왼쪽 긴 복도의 아래쪽**과 이어집니다.

- 상자는 **걸어서 밀면** 한 축으로만 미끄러집니다. 벽이나 책상에 닿으면 멈춥니다
- 색이 **다르면 들어가지 않습니다** — 빈칸 가까이 가도 그냥 지나갑니다
- 맞는 칸에 들어가면 딱 맞춰지고 더는 밀리지 않습니다

미는 판정은 `APushableBox::FollowPlayer`:

- 상자가 막고 있으면 플레이어 **속도는 0**이라 쓸 수 없습니다. 그래서
  `GetLastMovementInputVector`(가려는 방향)로 판정합니다
- 미는 속도는 `PushSpeed`(350 = 걷는 속도). 달려도 빨라지지 않습니다
- `MinContactFraction`(2/3): 미는 축과 **직각 방향으로 겹친 폭**이 둘 중 좁은
  쪽의 2/3 이상이어야 밀립니다. 어깨만 걸치면 안 밀립니다
- 빈칸에 들어가는 판정은 `AWordPuzzleRoom::SeatOverlapFraction`(0.8): 상자
  면적의 80% 이상이 칸 안에 들어와야 자리를 잡습니다
- 상자끼리는 **절대 겹치지 않습니다**. 앞에 붙은 상자는 줄줄이 같이 밀리고,
  줄의 맨 앞부터 움직입니다
- `MaxPushedCrates`(2)까지만 밀립니다. 3개가 줄서면 아무것도 안 움직입니다
- 2개를 밀 때는 `ChainSlowdown`(3)으로 나눈 속도입니다
- 자리를 잡은 상자는 벽 취급이라 밀리지 않습니다
- 밀어둔 위치는 `UDeceiveGameInstance::CratePositions`(색 id → 위치)에
  남습니다. 방을 나갔다 와도, 저장/불러오기를 해도 그 자리 그대로입니다

### 글자 바꾸기

`WordPuzzleRoom.cpp` 맨 위의 `WordRows` 가 글자 전부입니다.

```
TEXT("### ### ### #.#"),
TEXT("#1# 2.. #3# #.#"),   // 숫자 = 그 색 상자가 들어갈 빈칸
```

| 글자 | 뜻 |
| --- | --- |
| `#` | 책상 한 개 (한 칸) |
| `.` 또는 공백 | 빈 공간 |
| `1` / `2` / `3` | 빨강 / 초록 / 하늘 빈칸 |

줄 길이는 모두 같아야 하고, 상자 개수는 `SpawnContents()` 의 배열과 맞춰야
합니다. 책상 개수는 코드가 세어서 박스를 만드므로 따로 고칠 것이 없습니다.

**빈칸은 바깥에서 밀어 넣을 수 있는 자리**여야 합니다. 지금은 셋 다 글자의
맨 아랫줄이라 아래에서 위로 밀면 됩니다. 글자 안쪽에 구멍을 두면 상자를
넣을 길이 없어 풀 수 없게 됩니다.

| 바꾸고 싶은 것 | 어디서 |
| --- | --- |
| 방 크기 | `WordPuzzleRoom → Room Depth / Room Width` |
| 책상 한 칸 크기 | `Desk Cell` (상자도 이 크기에 맞춰 생성) |
| 문 위치 | `Exit Door Y` / `Entry Door Y` |
| 들어갈 때의 허용 오차 | `Snap Distance` |
| 대사 | `Crate_Seated`(알림) / `WordPuzzle_Solved` / `WordPuzzle_Locked` |

> 색은 지금 **디버그 박스**로만 표시됩니다 (조명이 없어 색 머티리얼이 까맣게
> 나오기 때문). 텍스처가 생기면 상자와 빈칸에 각각 입히면 됩니다.

---

## 3-7. 조각상 복도 — `StatueCorridor`

왼쪽 일자 복도(`NextStreet`)에 조각상 세 개가 왼쪽 벽을 따라 서 있습니다.
위에서부터 **초록 / 빨강 / 하늘색**이고, 상호작용하면 시계방향으로 90도씩
돕니다. 세 개가 **모두 오른쪽**을 보면 복도 위쪽 문이 열리고, 그 너머의
넓은 방(`StatueHall`)으로 갈 수 있습니다.

| 프로퍼티 | 기본값 | 역할 |
| --- | --- | --- |
| `RequiredFacing` | `Right` | 다 이쪽을 봐야 풀립니다 |
| `SolvedFlag` | `StatuePuzzle_Solved` | 위쪽 문이 기다리는 플래그 |
| `SolvedDialogueId` | `StatuePuzzle_Solved` | 풀렸을 때 나오는 대사 |
| `StatueInset` | `140` | 왼쪽 벽에서 얼마나 떨어져 서는지 |

- 조각상을 만지면 `Statue_Turn` 질문이 뜨고, **왼쪽/오른쪽** 중 고른 쪽으로
  90도 돕니다. 답은 `StatueTurn_<조각상 이름>` 플래그에 남습니다
- 조각상이 보는 방향은 `AStatue::Facing`(Up/Right/Down/Left). 돌린 뒤
  `Statue_Up` 같은 대사 한 줄이 알람으로 나옵니다
- 플래그로 잠긴 문(`RequiredFlag`)은 플래그가 **1일 때만** 열립니다. 0으로
  되돌리면 다시 잠깁니다
- 돌려둔 방향은 `UDeceiveGameInstance::StatueFacings` 에 남아, 방을 나갔다
  와도 저장/불러오기를 해도 그대로입니다
- 위쪽 문은 잠겨 있을 때 `StatueDoor` 대사가 나옵니다
- 문은 **지금 보고 있는 방향을 따라갑니다**. 하나라도 딴 데를 보면
  `SolvedFlag` 가 다시 0이 되어 문이 닫힙니다
- 방이 스폰한 액터는 반드시 `SpawnedContents` 에 넣어야 합니다. 안 넣으면
  맵을 옮겨도 안 사라지고 다음 맵에 그대로 남습니다

> 조각상도 색 디버그 박스입니다. 앞으로 뻗은 선이 보는 방향입니다.

### 방에 문을 더 다는 법

`ARoomArea` 는 기본 문(`DoorSide`) 하나에 더해 `ExtraDoors` 배열을 받습니다.
벽 한 면에 하나씩이고, 기본 문과 같은 면을 쓰면 무시됩니다.

```cpp
FRoomDoorway Door;
Door.Side = ERoomDoorSide::Back;      // Front / Back / Left / Right
Door.Area = EDeceiveArea::StatueHall;
Door.Entry = TEXT("FromStreet");      // 저쪽에서 들어설 이름
Door.EntryName = TEXT("FromHall");    // 이 문으로 돌아올 때 쓰는 이름
Door.Offset = -800.f;                 // 벽 가운데서 얼마나 옆으로 (0이면 가운데)
Door.RequiredFlag = TEXT("StatuePuzzle_Solved");  // 비우면 항상 열림
Door.LockedDialogueId = TEXT("StatueDoor");
Room->ExtraDoors.Add(Door);
```

`Offset` 이 다르면 **한 벽에 문 두 개**도 됩니다. 주황 방 아래쪽이 그렇게
가운데 문(십자맵 위로)과 왼쪽 끝의 작은 문(조각상 복도로)을 같이 답니다.

박스는 `ARoomArea::NumRoomBoxes`(20)개까지 쓰고 남는 건 꺼집니다. 방을
상속해서 박스를 더 쓰는 클래스(`LockedRoomArea`)는 이 상수 뒤부터 셉니다.

---

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
| 달리기 | Shift (누르고 있는 동안), 게임패드 L1 |
| 상호작용 / 다음 줄 / 확정 | Z, 게임패드 아래 버튼 |
| 취소 / 뒤로가기 | X, 게임패드 오른쪽 버튼 (선택지는 "다시 생각해보자."로 취소) |
| 메뉴 열기/닫기 | C, 게임패드 Start |
| 커서 이동 | ← → ↑ ↓ (3-3 표 참고) |

달리기 속도는 `SpriteCharacter → Walk Speed`(기본 350) × `Run Speed Multiplier`
(기본 1.5) 입니다.

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
