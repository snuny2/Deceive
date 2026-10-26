# Deceive — 진행 상황 정리 (2026-10-27)

HD-2D 스토리텔링 게임. 주인공 3명, 각각의 스토리.
현재는 **1단계 프로토타입**만 완료: 십자 골목길 + 움직이는 캐릭터 1명 + 교체 가능한 텍스처.

---

## 1. 환경

| 항목 | 값 |
| --- | --- |
| 엔진 | **UE 5.7** (`C:\Program Files\Epic Games\UE_5.7`) |
| 프로젝트 | `C:\Users\efzxc\Desktop\Deceive` |
| 모듈 | `Deceive` (C++ 런타임 모듈 1개) |
| 플러그인 | Paper2D (머티리얼 애셋만 사용) |
| Git | 미초기화 |

> 처음엔 UE 5.6으로 요청받았으나, 설치된 엔진이 5.7뿐이라 `EngineAssociation`을 5.7로 맞췄음.

---

## 2. 핵심 제약 — 왜 전부 C++인가

Claude Code는 텍스트 파일만 생성할 수 있어서 **언리얼 바이너리 애셋을 만들 수 없음**:

- `.umap` (레벨)
- `.uasset` (머티리얼, 스프라이트, 텍스처, 블루프린트)
- Enhanced Input의 `InputAction` / `InputMappingContext`

그래서 다음과 같이 우회했고, 이게 현재 구조의 거의 모든 설계 결정을 지배함:

| 원래 필요한 것 | 대신 쓴 방법 |
| --- | --- |
| 레벨에 배치된 골목길 지오메트리 | `ACrossAlley`가 박스 메시 14개를 런타임 생성 |
| 레벨에 배치된 PlayerStart | `ADeceiveGameMode::InitGame`이 런타임 생성 |
| 커스텀 머티리얼 애셋 | Paper2D 내장 머티리얼 + `UMaterialInstanceDynamic` |
| Enhanced Input 애셋 | `Config/DefaultInput.ini` 레거시 축 매핑 |

**결과: 빈 레벨 하나만 있으면 플레이됨.** 레벨에 아무것도 배치할 필요 없음.

---

## 3. 파일 구조

```
Deceive/
├── Deceive.uproject
├── Config/
│   ├── DefaultEngine.ini      # GlobalDefaultGameMode = DeceiveGameMode
│   ├── DefaultGame.ini
│   └── DefaultInput.ini       # WASD / 방향키 / 게임패드 좌스틱
├── Source/
│   ├── Deceive.Target.cs
│   ├── DeceiveEditor.Target.cs
│   └── Deceive/
│       ├── Deceive.Build.cs            # Core, CoreUObject, Engine, InputCore
│       ├── Deceive.cpp                 # IMPLEMENT_PRIMARY_GAME_MODULE
│       ├── DeceiveGameMode.h/.cpp      # 골목길 + PlayerStart 런타임 스폰
│       ├── CrossAlley.h/.cpp           # 십자 골목길 (박스 14개)
│       ├── SpriteCharacter.h/.cpp      # 빌보드 스프라이트 + 이동 + 카메라
│       └── DeceiveSpriteMaterial.h/.cpp # 텍스처 교체용 MID 생성 헬퍼
├── README.md
└── HANDOFF.md                 # 이 문서
```

---

## 4. 구현된 것

### 4-1. 십자 골목길 — `ACrossAlley`

`/Engine/BasicShapes/Cube` 14개로 구성. `OnConstruction`에서 위치·스케일·머티리얼을 매번 계산하므로 디테일 패널에서 값을 바꾸면 즉시 반영됨.

| 구성 | 개수 | 인덱스 |
| --- | --- | --- |
| 바닥 슬랩 (X방향 + Y방향, 겹침) | 2 | `Box0`, `Box1` |
| 복도 벽 (교차점에서 끊김) | 8 | `Box2`~`Box9` |
| 갈래 끝막이 | 4 | `Box10`~`Box13` |

조절 가능한 치수 (`EditAnywhere`):

| 프로퍼티 | 기본값 |
| --- | --- |
| `CorridorWidth` | 400 |
| `ArmLength` | 1200 (교차점 중심 → 갈래 끝) |
| `WallHeight` | 600 |
| `WallThickness` | 100 |
| `FloorThickness` | 20 |

### 4-2. 캐릭터 — `ASpriteCharacter`

`ACharacter` 상속. `/Engine/BasicShapes/Plane`을 세워서 **Yaw만 빌보딩**(카메라를 향해 회전).

- 기본 회전 `FRotator(0, 90, -90)` → 노멀 -X, 텍스처 U축은 +Y, V축은 아래
- Rotator는 Yaw가 가장 바깥이라, Yaw만 더하면 방향이 유지된 채 카메라를 향함
- 충돌은 캡슐이 담당, 쿼드는 `NoCollision`
- `MaxWalkSpeed` 350, `bOrientRotationToMovement` false

| 프로퍼티 | 기본값 |
| --- | --- |
| `SpriteWidth` | 120 |
| `SpriteHeight` | 180 |
| `SpriteTexture` | (미지정) |
| `BaseMaterial` | (미지정 → Paper2D 기본) |

### 4-3. 카메라

스프링암이 **Yaw 0 고정**(월드 -X 뒤에서 +X를 바라봄). `SetUsingAbsoluteRotation(true)`라 캐릭터 회전을 따라가지 않음.

| 항목 | 값 |
| --- | --- |
| `TargetArmLength` | **2700** |
| 스프링암 Pitch | **-25°** |
| `FieldOfView` | 40 |
| `bDoCollisionTest` | false |

> 이 세팅이면 가로 시야폭이 약 1965라 골목 전체 폭 2400을 한 화면에 못 담음.
> **의도된 상태** — 이동하면서 보는 방식으로 가기로 결정함.

### 4-4. 벽이 화면을 가리지 않게 — `bHideCameraSideWalls`

카메라 Yaw가 0으로 고정이므로 가리는 벽이 **-X 쪽으로 정적으로 확정**됨. `SetHiddenInGame`만 사용 → **충돌은 그대로 남아 이동은 똑같이 막힘**. 에디터 뷰포트에서는 편집 편의를 위해 계속 보임.

숨겨지는 박스 5개:

| 박스 | 정체 |
| --- | --- |
| `Box2`, `Box4` | -X 갈래의 양옆 벽 (안 숨기면 카메라가 터널 속을 보게 됨) |
| `Box3`, `Box7` | 남/북 갈래의 카메라 쪽 벽 |
| `Box10` | -X 끝막이 |

### 4-5. 텍스처 교체

Paper2D 내장 머티리얼 + `UMaterialInstanceDynamic`. 텍스처 파라미터 이름은 **`SpriteTexture`**.

| 대상 | 기본 머티리얼 | 교체 프로퍼티 |
| --- | --- | --- |
| 캐릭터 | `MaskedUnlitSpriteMaterial` (컷아웃) | `ASpriteCharacter::SpriteTexture` |
| 바닥 | `OpaqueUnlitSpriteMaterial` (불투명) | `ACrossAlley::FloorTexture` |
| 벽 | `OpaqueUnlitSpriteMaterial` (불투명) | `ACrossAlley::WallTexture` |

- 전부 **언릿** → 레벨에 라이트가 없어도 보임
- 각 클래스의 `BaseMaterial`에 직접 만든 머티리얼을 꽂으면 기본값을 덮어씀
- 텍스처 미지정 시 `/Engine/EngineResources/DefaultTexture`가 플레이스홀더로 들어감

에디터에서 만지려면 `SpriteCharacter` / `CrossAlley` 상속 블루프린트를 만들고, `DeceiveGameMode` 상속 블루프린트의 `DefaultPawnClass` / `AlleyClass`에 지정하는 게 가장 편함.

---

## 5. 검증 상태

### 검증된 것

- `DeceiveEditor Win64 Development` 빌드 성공 (프로젝트 코드 경고 0)
- `Deceive Win64 Development` (게임 타깃) 빌드 + 링크 성공
- 헤드리스 실행에서 `CrossAlley_0`, `SpriteCharacter_0` 스폰 확인, 크래시 없음
- `GetAll` 로그로 확인: `TargetArmLength` 2700, Pitch -25°, FOV 40
- `GetAll` 로그로 확인: 14개 중 정확히 5개(`Box2,3,4,7,10`)가 `bHiddenInGame = True`
- `GetAll` 로그로 확인: MID 15개 전부 정상 적용 (박스 14 + 쿼드 1)
- D3D12 실제 렌더링 스크린샷 확보 → **캐릭터 스프라이트가 화면 중앙에 똑바로 선 직사각형으로 렌더링됨** (빌보드 회전과 UV 상하 방향 정상)

### 검증 안 된 것

- 키보드 입력으로 실제 이동/충돌이 되는지 (헤드리스라 입력 시뮬레이션 미실시)
- 에디터 뷰포트에서 `OnConstruction` 실시간 반영
- 실제 텍스처를 넣었을 때의 모양 (플레이스홀더로만 확인)

### 검증용 명령 (재현용)

```
# 헤드리스 상태 덤프
UnrealEditor-Cmd.exe Deceive.uproject /Engine/Maps/Entry -game -nullrhi -unattended ^
  -ExecCmds="GetAll StaticMeshComponent bHiddenInGame,quit"

# 스크린샷 (프로세스를 외부에서 45초 뒤 종료해야 함)
UnrealEditor-Cmd.exe Deceive.uproject /Engine/Maps/Entry -game -windowed -resx=1280 -resy=720 ^
  -unattended -ExecCmds="HighResShot 1280x720"
```

주의: `-game` 콘솔에는 `sleep` 명령이 없음. `viewmode wireframe` + `HighResShot` 조합은 검은 화면이 나옴.

---

## 6. 개발 중 걸린 함정

| 함정 | 해결 |
| --- | --- |
| 에디터가 켜져 있으면 Live Coding이 빌드를 막음 | 에디터 종료 후 빌드. 헤더 변경은 핫리로드 불가, 재시작 필수 |
| Git Bash가 `/Engine/Maps/Entry`를 윈도우 경로로 변환 | `MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL="*"` |
| `-ExecCmds=HighResShot 1280x720` 공백에서 인자 분리 | 통째로 따옴표: `-ExecCmds="HighResShot 1280x720"` |
| `DisplayAll`은 화면 출력이라 nullrhi에서 안 잡힘 | 로그로 찍히는 `GetAll` 사용 |
| `GetAll`의 배열 프로퍼티는 다음 줄에 출력됨 | 컨텍스트까지 봐야 함 (안 그러면 "비어있다"고 오판) |
| 생성자에서 `LoadObject` | `ConstructorHelpers::FObjectFinder` 사용 |

---

## 7. 아직 없는 것 — 다음에 정할 것들

### 바로 이어지는 기술 작업

- 스프라이트 애니메이션 (idle / walk), 이동 방향에 따른 좌우 반전
- HD-2D 룩: 틸트시프트 피사계심도, 포스트프로세스, 라이팅, 블룸
- Enhanced Input으로 전환 (상호작용 키, 대화 진행 키 등 추가 시)
- 상호작용 시스템 (NPC, 오브젝트)

### 게임 설계 — 아직 전혀 정해진 바 없음

- **주인공 3명이 각각 누구인지**, 각자의 스토리가 무엇인지
- 3개 스토리의 관계: 평행인가, 교차하는가, 시간순인가
- 플레이어가 셋 사이를 **어떻게 전환**하는가 (챕터제 / 자유 전환 / 분기)
- "Deceive"라는 제목이 가리키는 것 — 속임수가 플레이 메커닉인가, 서사 주제인가
- 십자 골목길이 어떤 장소인가 (허브인가, 특정 챕터의 무대인가)
- 대화 시스템 형태, 선택지 유무, 분기 데이터 구조

> 프로젝트 이름이 "Deceive"이고 주인공이 3명인 점을 보면, 같은 사건을 세 관점에서
> 보여주며 서로의 진술이 어긋나는 구조를 염두에 두신 걸로 보이지만 **확인된 바 없음**.
> 여기서부터 Claude 채팅에서 정리하면 될 듯.

---

## 8. 실행 방법 요약

1. `Deceive.uproject` 우클릭 → Generate Visual Studio project files
2. 빌드 (Development Editor / Win64) 또는 `.uproject` 더블클릭 후 리빌드 프롬프트에 Yes
3. File → New Level → **Empty Level**, 저장
4. Project Settings → Maps & Modes에서 Editor/Game Default Map을 그 레벨로 지정
5. Play — WASD / 방향키 / 게임패드 좌스틱

빠른 확인만 원하면 레벨 안 만들고:
`UnrealEditor-Cmd.exe Deceive.uproject /Engine/Maps/Entry -game`
