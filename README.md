# 정원 그리기 MFC Dialog 프로젝트

이 프로젝트는 MFC Dialog 기반으로 작성된 Windows 애플리케이션으로, 사용자가 클릭한 3개의 점을 기반으로 원(정원)을 그리고, 다양한 UI 상호작용(드래그, 초기화, 랜덤 이동 등)을 구현합니다.  
특히, CDC 클래스를 사용하지 않고 순수 Win32 GDI 함수를 이용하여 그리기를 수행하며, 깜빡임 문제를 최소화하기 위해 더블 버퍼링 및 WM_ERASEBKGND 오버라이드를 적용하였습니다.

## 주요 기능

- **클릭 지점 원 그리기**  
  - 사용자가 대화상자에서 클릭한 좌표에 사용자 지정 반지름의 작은 원을 그립니다.  
  - 각 클릭 지점의 좌표는 Static 컨트롤에 실시간으로 표시됩니다.

- **정원(3점을 지나는 원) 그리기**  
  - 세 번째 클릭 이후, 입력된 3개 점을 모두 지나가는 원(정원)을 그립니다.  
  - 정원은 내부가 채워지지 않으며, 사용자가 입력한 외곽선 두께를 적용합니다.

- **동적 업데이트**  
  - 3개의 클릭 지점 중 하나를 클릭하여 드래그하면, 해당 점이 이동되고 정원이 실시간으로 재계산되어 업데이트됩니다.

- **초기화 기능**  
  - [초기화] 버튼을 클릭하면 지금까지 그려진 모든 내용을 삭제하고, 새로 3개의 클릭 지점을 입력받을 수 있도록 초기 상태로 복원합니다.

- **랜덤 이동 기능**  
  - [랜덤 이동] 버튼을 누르면, 이미 입력된 3개의 점이 별도의 쓰레드에서 초당 2회(0.5초 간격), 총 10회 랜덤하게 이동하며 정원이 다시 그려집니다.  
  - 이 과정에서 UI 프리징 없이 부드럽게 업데이트됩니다.

## 개발 환경

- **개발 도구:** Visual Studio (MFC 지원)
- **플랫폼:** Windows
- **언어:** C++ (MFC)

## 빌드 및 실행 방법

1. Visual Studio에서 새 MFC Dialog 기반 프로젝트로 생성합니다.
2. 제공된 소스 파일(`MyDialog.h`, `MyDialog.cpp` 등)을 프로젝트에 추가합니다.
3. 리소스 에디터를 이용해 다음 컨트롤들을 추가 및 ID를 설정합니다:
   - Edit 컨트롤: `IDC_EDIT_POINT_RADIUS` (클릭 지점 원의 반지름 입력)
   - Edit 컨트롤: `IDC_EDIT_GARDEN_THICKNESS` (정원 외곽선 두께 입력)
   - Static 컨트롤: `IDC_STATIC_POINT1`, `IDC_STATIC_POINT2`, `IDC_STATIC_POINT3` (각 클릭 좌표 표시)
   - 버튼: `IDC_BUTTON_INIT` ([초기화] 버튼)
   - 버튼: `IDC_BUTTON_RANDOM` ([랜덤 이동] 버튼)
4. 소스 코드 내에 WM_PAINT, WM_ERASEBKGND, WM_CTLCOLOR, 마우스 이벤트(ON_LBUTTONDOWN, ON_MOUSEMOVE, ON_LBUTTONUP) 및 랜덤 이동 쓰레드 등의 메시지 처리기를 구현합니다.
5. 프로젝트를 빌드한 후, 실행하여 기능을 테스트합니다.

## 프로젝트 구조

- **MyDialog.h / MyDialog.cpp:**  
  - 대화상자 클래스 구현 파일  
  - 클릭 지점, 정원 그리기, 드래그, 초기화, 랜덤 이동 등의 기능 구현

- **Resource 파일:**  
  - Dialog 리소스 및 컨트롤 ID 설정

## 주의 사항

- 이 프로젝트는 CDC 클래스 대신 Win32 GDI 함수를 사용하여 그리기를 구현하였습니다.
- 깜빡임을 최소화하기 위해 더블 버퍼링, WM_ERASEBKGND 오버라이드, WS_CLIPCHILDREN/WS_EX_COMPOSITED 스타일 등을 적용하였습니다.
- Static 컨트롤의 투명 배경 처리는 WM_CTLCOLOR 메시지 핸들러와 함께 구현되어 있습니다.

## 라이선스

해당 프로젝트는 본인의 학습용 과제로 제공되며, 별도의 라이선스가 부여되지 않습니다.

---

프로젝트에 대한 추가 질문이나 개선 사항이 있다면 언제든지 문의해 주세요.
