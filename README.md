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
5. Play. WASD / 방향키 / 게임패드 왼쪽 스틱으로 이동

레벨에 아무것도 넣지 않아도 `ADeceiveGameMode::InitGame` 이 골목길과 스폰 지점을
런타임에 생성합니다.

빠르게 확인만 하고 싶으면 레벨을 만들지 않고 엔진 기본 맵으로 바로 실행해도 됩니다:
`UnrealEditor-Cmd.exe Deceive.uproject /Engine/Maps/Entry -game`

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
