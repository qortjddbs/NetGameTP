프로젝트 개요

이 리포지토리는 OpenGL/GLUT 기반의 간단한 3D Bounce Ball 게임(Computer Graphics Term Project)입니다. 주요 역할은 렌더링, 입력/출력 처리, 카메라 제어, 게임 월드 업데이트 등입니다.

파일 및 연결 관계 (간단 요약)

- `main.cpp`
  - 프로그램 진입점. OpenGL/GLUT 초기화, 셰이더 로드/컴파일, 주요 객체 생성 및 루프 등록을 수행합니다.
  - 포함된 헤더: `Headers.h`, `Vertex.h`, `Axis.h`, `ObjModel.h`, `GameWorld.h`, `InputHandler.h`, `OutputHandler.h`, `Camera.h`.
  - 셰이더 파일(`Phong_vertex.glsl`, `Phong_fragment.glsl`)을 실행 디렉터리에 두어야 합니다(파일 로드 함수 `filetobuf` 사용).
  - 전역으로 선언된 설정 변수(조명, 렌더링 옵션, 윈도우 크기 등)를 통해 입력 핸들러와 출력 핸들러가 상태를 공유합니다.
  - 주요 역할: 윈도우/셰이더/VAO/VBO 초기화, 카메라 및 GameWorld 생성, GLUT 콜백(draw, reshape, keyboard, idle) 등록.

- `Headers.h`
  - 공통 헤더(라이브러리 포함, 전처리기 정의 등)를 모아두는 파일입니다. 프로젝트 전역에서 필요한 OpenGL/GLM/GLUT/GLEW 등의 include를 포함할 가능성이 큽니다.

- `Vertex.h`
  - 정점 구조체(`VERTEX`) 정의와 정점 관련 유틸리티가 들어있습니다.
  - `main.cpp`에서 VAO/VBO 구성 시 `VERTEX` 크기를 사용합니다.

- `Axis.h` / `Axis.cpp`
  - 좌표축(AXIS) 렌더링을 담당하는 클래스입니다. 셰이더 프로그램 ID를 받아 초기화(`init`)하고 `Draw()`로 축을 렌더링합니다.
  - `main.cpp`는 `Axis.init(shaderProgramID)` 및 `Axis.Draw()`를 호출합니다.

- `ObjModel.h` / `ObjModel.cpp`
  - .obj 모델 로더 및 모델 렌더링 클래스입니다. Obj 파일을 읽어 정점/노멀/텍스처 좌표를 파싱하고 GPU로 전송합니다.
  - 쉐이더의 속성 위치 및 매트릭스 유니폼과 호환되도록 구현되어야 합니다.

- `GameWorld.h` / `GameWorld.cpp`
  - 게임 전체 상태(플레이어, 장애물, 충돌 검사, 물리 업데이트 등)를 관리합니다.
  - `GameWorld::initialize()`로 초기 환경을 생성하고 `update(float dt)`로 매 프레임 상태를 갱신하며 `draw()`로 모든 게임 오브젝트를 렌더링합니다.
  - `main.cpp`에서 `GameWorld` 인스턴스를 생성하여 게임 루프에 연결합니다.

- `InputHandler.h` / `InputHandler.cpp`
  - 키보드 및 마우스 입력을 처리하여 게임 상태(플레이어 이동, 토글 옵션 등)에 반영합니다.
  - `main.cpp`에서 `inputHandler->setGameWorld(gameWorld)` 및 `setCamera(camera)` 등을 호출하여 참조를 제공합니다.
  - 키 입력 콜백(`KeyBoardDown`)에서 `inputHandler->handleKeyboard`를 호출합니다.

- `OutputHandler.h` / `OutputHandler.cpp`
  - 키 업 이벤트 등 일부 입력을 받아 게임/렌더링 설정을 변경하거나 디버그 출력을 담당합니다.
  - `main.cpp`에서 `outputHandler->setGameWorld(gameWorld)` 및 `setCamera(camera)` 등을 호출합니다.

- `Camera.h` / `Camera.cpp`
  - 카메라 위치, 타겟, yaw/pitch/roll 제어 및 뷰/프로젝션 매트릭스를 계산합니다.
  - `camera->getViewMatrix()` 및 `camera->getPosition()` 등을 통해 `main.cpp`에서 셰이더로 전달됩니다.

- `Phong_vertex.glsl` / `Phong_fragment.glsl`
  - Phong 조명 모델을 구현한 버텍스 및 프래그먼트 셰이더 파일입니다. `main.cpp`의 `filetobuf`로 읽혀 컴파일됩니다.
  - 셰이더 내부의 유니폼 이름(`lightPos`, `lightColor`, `turnOnLight`, `view`, `proj`, `modelMat`, `viewPos`, `lightIntensity`, `objectColor` 등)은 `main.cpp`와 일치해야 합니다.

빌드 및 실행

1. 요구사항
   - Visual Studio (또는 C++14 호환 컴파일러)
   - GLUT (freeglut), GLEW, GLM 라이브러리
   - 위 라이브러리들이 프로젝트 설정(링커, include 디렉토리)에 올바르게 추가되어야 합니다.

2. 실행 순서(개발 환경에서)
   - 셰이더 파일(`Phong_vertex.glsl`, `Phong_fragment.glsl`)을 실행 파일(.exe)과 같은 폴더에 배치하세요 또는 `main.cpp`의 경로를 변경하세요.
   - 솔루션을 빌드하고 실행하면 윈도우가 열리며 게임이 시작됩니다.

주요 컨트롤 (프로그램 내부 출력에 표기)

- W/A/S/D : 플레이어 이동
- Space : 점프
- C : 게임 시작/일시정지
- X : 게임 리셋
- H : Depth Test 토글
- U : Cull Face 토글
- P/p : 투영 모드 전환(원근/직교)
- R : 조명 토글
- + / - : 조명 세기 조절
- Q : 종료

확장 및 디버그 포인트

- 셰이더 컴파일 에러: `main.cpp`의 `make_vertexShaders`/`make_fragmentShaders`에서 콘솔 에러 메시지를 확인하세요.
- 모델 파일(.obj) 로드 문제: `ObjModel` 클래스의 경로 또는 파일 포맷 호환성을 점검하세요.
- 카메라 동작 문제: `Camera` 클래스의 `orbitAroundTarget`, `getViewMatrix` 구현을 확인하세요.

간단 요약

- `main.cpp`가 중앙 허브이며, 다른 모듈들은 게임 월드, 입력/출력, 카메라, 모델 로딩, 축 렌더링, 셰이더로 기능이 분리되어 있습니다.
- 각 헤더/소스 파일은 해당 기능(렌더링/입력/물리/카메라 등)을 담당하므로 해당 모듈을 찾아 수정/확장하면 됩니다.

파일 위치 예시

- 루트 디렉터리: `main.cpp`, `Headers.h`, `Phong_vertex.glsl`, `Phong_fragment.glsl`, `README.txt`
- 서브 디렉터리(선택적): `src/` 또는 `models/`(obj 파일), `shaders/`(셰이더 파일)

문의사항

- 특정 파일의 내부 구현을 상세히 문서화하기 원하면, 어떤 파일을 우선적으로 설명할지 알려주십시오.

====================================================================================================================================================

클래스 상속(상속성) 기본 개념 및 ParentModel/자식 클래스 사용 안내

- 상속의 기본
  - 상속(Inheritance)은 기존 클래스(부모/기반 클래스, Parent/Superclass)의 인터페이스와 동작을 재사용하고 확장하기 위한 객체지향 기법입니다.
  - 자식 클래스(파생 클래스, Child/Subclass)는 부모 클래스의 멤버(필드/메서드)를 물려받고, 필요하면 재정의(override)하거나 새로운 기능을 추가할 수 있습니다.
  - 접근 지정자(public/protected/private)를 통해 상속된 멤버에 대한 접근 범위를 제어합니다. 보통 인터페이스를 유지하려면 `public` 상속을 사용합니다.

- 가상 함수와 다형성
  - 부모 클래스의 메서드를 `virtual`로 선언하면 자식 클래스에서 재정의할 수 있고, 부모 타입 포인터로 호출할 때 자식 구현이 실행됩니다(런타임 다형성).
  - 베이스 클래스에는 반드시 `virtual` 소멸자(`virtual ~ParentModel() = default;`)를 선언하여 자식 객체가 올바르게 소멸되도록 해야 합니다.
  - C++11 이후에는 오버라이드하는 메서드에 `override` 키워드를 붙여 컴파일러가 시그니처를 검사하도록 하는 것이 안전합니다.

- 객체 슬라이싱(object slicing) 방지
  - 객체를 값으로 저장하면(기본 타입처럼) 자식 클래스의 추가 멤버가 잘려나갑니다. 따라서 부모형 포인터(`ParentModel*`)나 스마트포인터(`std::unique_ptr<ParentModel>` / `std::shared_ptr<ParentModel>`)로 관리해야 합니다.

- ParentModel (프로젝트 관점의 권장 설계)
  - 목적: 모든 게임 오브젝트(모델)가 공통으로 가져야 할 인터페이스를 정의합니다.
  - 권장 멤버(예)
    - 가상 소멸자: `virtual ~ParentModel() = default;`
    - 렌더/업데이트 인터페이스: `virtual void initialize();`, `virtual void update(float dt);`, `virtual void draw();`
    - 위치/변환 접근자: `virtual glm::vec3 getTranslation() const;`, `virtual void setTranslation(const glm::vec3& pos);`
    - 충돌/상태 인터페이스(필요시): `virtual BoundingBox getBounds() const;`
  - 구현 팁: 기본 동작(정적 데이터 설정 등)은 ParentModel에 구현하고, 오브젝트별 고유 동작만 자식에서 재정의하세요.

- 자식 클래스들(예: PlayerModel, BallModel, ObstacleModel 등)
  - 역할: ParentModel의 공통 인터페이스를 상속받아 각 오브젝트만의 동작(물리, 입력 반응, 특수 렌더링)을 구현합니다.
  - 예시 구현
    - `class PlayerModel : public ParentModel { public: void update(float dt) override; void draw() override; /* 점프/이동 상태 등 */ }`
    - `class BallModel : public ParentModel { public: void update(float dt) override; void draw() override; /* 반사/충돌 처리 */ }`
  - 자식 클래스는 추가 상태(속도, 가속도, 스코어 등)를 멤버 변수로 갖고, `update`에서 물리/행동을 갱신합니다.

- GameWorld와의 결합(사용법)
  - `GameWorld`는 `std::vector<std::unique_ptr<ParentModel>> objects;` 같은 컨테이너에 부모형 스마트포인터로 모든 오브젝트를 보관합니다.
  - 매 프레임 `for (auto& obj : objects) obj->update(deltaTime);` 및 `obj->draw();`처럼 부모 포인터를 통해 호출하면 각 자식의 재정의된 메서드가 실행됩니다.
  - 특정 타입의 추가 기능이 필요하면 `dynamic_cast`로 안전하게 다운캐스트하거나, 컴포지션/시스템 패턴을 사용하는 것이 좋습니다.

- 안전과 유지보수 권장사항
  - 모든 가상 함수에 `override`를 사용하세요.
  - 자원 관리는 스마트 포인터로 하고, `new/delete` 직접 관리는 피하세요.
  - 인터페이스(ParentModel)는 가능한 작게 유지하고, 구체적 행동은 자식 클래스 또는 별도 시스템에 위임하세요.

요약

ParentModel(베이스 클래스)은 게임 오브젝트의 공통 인터페이스를 정의하고, 자식 클래스들은 각자의 동작을 구현합니다. 
런타임 다형성(virtual 함수)을 사용하면 `GameWorld`처럼 오브젝트 컬렉션을 부모형 포인터로 관리하면서 각 오브젝트의 고유 동작을 자연스럽게 호출할 수 있습니다. 
안전한 소멸과 자원 관리를 위해 `virtual` 소멸자와 스마트 포인터 사용을 권장합니다.

====================================================================================================================================================