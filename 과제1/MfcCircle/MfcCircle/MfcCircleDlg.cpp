
// MfcCircleDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "MfcCircle.h"
#include "MfcCircleDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMfcCircleDlg 대화 상자



CMfcCircleDlg::CMfcCircleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCCIRCLE_DIALOG, pParent),
    m_nClickCount(0),
    m_bDragging(false),
    m_nDraggedIndex(-1),
    m_pRandomThread(nullptr),
    m_bRandomThreadRunning(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    for (int i = 0; i < 3; i++) {
        m_bPointSet[i] = false;
    }
    // 기본값 (에디트 박스와 동기화하거나 사용자가 변경 가능)
    m_nPointRadius = 20;
    m_nGardenThickness = 3;

    // 난수 초기화
    srand((unsigned int)time(NULL));

    InitializeCriticalSection(&m_csPoints);
}

CMfcCircleDlg::~CMfcCircleDlg()
{
    DeleteCriticalSection(&m_csPoints);
}

void CMfcCircleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
    // 에디트 컨트롤과 변수 연결 (리소스 ID에 맞게)
    DDX_Text(pDX, IDC_EDIT_POINT_RADIUS, m_nPointRadius);
    DDX_Text(pDX, IDC_EDIT_GARDEN_THICKNESS, m_nGardenThickness);
}


BEGIN_MESSAGE_MAP(CMfcCircleDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_BN_CLICKED(IDC_BUTTON_INIT, &CMfcCircleDlg::OnBnClickedInit)
    ON_BN_CLICKED(IDC_BUTTON_RANDOM, &CMfcCircleDlg::OnBnClickedRandom)
    ON_MESSAGE(WM_UPDATE_DRAW, &CMfcCircleDlg::OnUpdateDraw)
    ON_EN_CHANGE(IDC_EDIT_POINT_RADIUS, &CMfcCircleDlg::OnEnChangePointRadius)
    ON_EN_CHANGE(IDC_EDIT_GARDEN_THICKNESS, &CMfcCircleDlg::OnEnChangeGardenThickness)
END_MESSAGE_MAP()

BOOL CMfcCircleDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
    SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

    ModifyStyle(0, WS_CLIPCHILDREN);

    // 초기 UI 설정(예: Static 컨트롤에 빈 좌표 표시)
    UpdatePointCoordinatesUI();
    return TRUE;
}

//
// OnPaint: CDC 대신 HDC와 순수 GDI 함수를 사용하여 그리기
//
void CMfcCircleDlg::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(m_hWnd, &ps);
    // 전체 클라이언트 영역을 가져옵니다.
    RECT rcClient;
    ::GetClientRect(m_hWnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // off-screen 버퍼를 위한 메모리 DC 생성
    HDC memDC = ::CreateCompatibleDC(hdc);
    HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)::SelectObject(memDC, memBitmap);

    // 메모리 DC의 원점을 (0,0)으로 설정 (필요한 경우)
    ::SetViewportOrgEx(memDC, 0, 0, NULL);

    // off-screen 버퍼를 배경 색으로 채우기 (예: 흰색)
    HBRUSH hBrush = (HBRUSH)::GetStockObject(WHITE_BRUSH);
    ::FillRect(memDC, &rcClient, hBrush);

    // 그리기 전에 임계 구역 획득
    EnterCriticalSection(&m_csPoints);

    // 클릭 지점 원 그리기
    for (int i = 0; i < m_nClickCount; i++)
    {
        CPoint pt = m_points[i];
        int left = pt.x - m_nPointRadius;
        int top = pt.y - m_nPointRadius;
        int right = pt.x + m_nPointRadius;
        int bottom = pt.y + m_nPointRadius;
        ::Ellipse(memDC, left, top, right, bottom);
    }

    // 세 점이 모두 입력되었다면 정원(세 점을 지나는 원) 그리기
    if (m_nClickCount == 3)
    {
        CPoint gardenCenter;
        int gardenRadius;
        if (CalculateGardenCircle(gardenCenter, gardenRadius))
        {
            // 사용자 입력 두께로 펜 생성
            HPEN hPen = ::CreatePen(PS_SOLID, m_nGardenThickness, RGB(0, 0, 0));
            HPEN hOldPen = (HPEN)::SelectObject(memDC, hPen);
            // 내부 채우지 않도록 NULL 브러시 선택
            HBRUSH hOldBrush = (HBRUSH)::SelectObject(memDC, ::GetStockObject(NULL_BRUSH));

            int left = gardenCenter.x - gardenRadius;
            int top = gardenCenter.y - gardenRadius;
            int right = gardenCenter.x + gardenRadius;
            int bottom = gardenCenter.y + gardenRadius;
            ::Ellipse(memDC, left, top, right, bottom);

            ::SelectObject(memDC, hOldBrush);
            ::SelectObject(memDC, hOldPen);
            ::DeleteObject(hPen);
        }
    }
    LeaveCriticalSection(&m_csPoints);

    // 메모리 DC에 그린 내용을 화면 DC로 복사 (BitBlt)
    ::BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // 리소스 정리
    ::SelectObject(memDC, oldBitmap);
    ::DeleteObject(memBitmap);
    ::DeleteDC(memDC);

    ::EndPaint(m_hWnd, &ps);
}

// 세 점을 지나는 원(정원) 계산
bool CMfcCircleDlg::CalculateGardenCircle(CPoint& center, int& radius)
{
    if (m_nClickCount < 3)
        return false;

    double x1 = m_points[0].x, y1 = m_points[0].y;
    double x2 = m_points[1].x, y2 = m_points[1].y;
    double x3 = m_points[2].x, y3 = m_points[2].y;

    double denominator = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (fabs(denominator) < 1e-6)
        return false; // 세 점이 공선

    double a = ((x1 * x1 + y1 * y1) * (y2 - y3) +
        (x2 * x2 + y2 * y2) * (y3 - y1) +
        (x3 * x3 + y3 * y3) * (y1 - y2)) / denominator;
    double b = ((x1 * x1 + y1 * y1) * (x3 - x2) +
        (x2 * x2 + y2 * y2) * (x1 - x3) +
        (x3 * x3 + y3 * y3) * (x2 - x1)) / denominator;

    center.x = (int)a;
    center.y = (int)b;
    radius = (int)sqrt((x1 - a) * (x1 - a) + (y1 - b) * (y1 - b));
    return true;
}

// 클릭 좌표를 Static 컨트롤에 업데이트 (예: IDC_STATIC_POINT1, IDC_STATIC_POINT2, IDC_STATIC_POINT3)
void CMfcCircleDlg::UpdatePointCoordinatesUI()
{
    CString str;
    for (int i = 0; i < 3; i++)
    {
        if (m_bPointSet[i] || i < m_nClickCount)
        {
            EnterCriticalSection(&m_csPoints);
            str.Format(_T("Point %d: (%d, %d)"), i + 1, m_points[i].x, m_points[i].y);
            LeaveCriticalSection(&m_csPoints);
        }
        else
        {
            str.Format(_T("Point %d: ( , )"), i + 1);
        }
        // 리소스 ID를 연속으로 설정
        GetDlgItem(IDC_STATIC_POINT1 + i)->SetWindowText(str);
    }
}

//
// 마우스 이벤트 처리
//
void CMfcCircleDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_nClickCount < 3)
    {
        EnterCriticalSection(&m_csPoints);
        m_points[m_nClickCount] = point;
        m_bPointSet[m_nClickCount] = true;
        m_nClickCount++;
        LeaveCriticalSection(&m_csPoints);
        UpdatePointCoordinatesUI();
        Invalidate();
    }
    else
    {
        // 세 점이 모두 존재하면 드래그 모드로 전환: 클릭한 위치가 기존 점 내에 있는지 확인
        for (int i = 0; i < 3; i++)
        {
            EnterCriticalSection(&m_csPoints);
            CPoint pt = m_points[i];
            LeaveCriticalSection(&m_csPoints);
            int dx = point.x - pt.x;
            int dy = point.y - pt.y;
            if (dx * dx + dy * dy <= m_nPointRadius * m_nPointRadius)
            {
                m_bDragging = true;
                m_nDraggedIndex = i;
                m_ptLastDragPos = point;
                break;
            }
        }
    }
    CDialogEx::OnLButtonDown(nFlags, point);
}

void CMfcCircleDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_bDragging && m_nDraggedIndex >= 0 && m_nDraggedIndex < 3)
    {
        int dx = point.x - m_ptLastDragPos.x;
        int dy = point.y - m_ptLastDragPos.y;
        EnterCriticalSection(&m_csPoints);
        m_points[m_nDraggedIndex].x += dx;
        m_points[m_nDraggedIndex].y += dy;
        LeaveCriticalSection(&m_csPoints);
        m_ptLastDragPos = point;
        UpdatePointCoordinatesUI();
        Invalidate();

    }
    CDialogEx::OnMouseMove(nFlags, point);
}

void CMfcCircleDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_bDragging = false;
    m_nDraggedIndex = -1;
    CDialogEx::OnLButtonUp(nFlags, point);
}

//
// [초기화] 버튼: 모든 내용을 삭제하고 초기 상태로 복원
//
void CMfcCircleDlg::OnBnClickedInit()
{
    // 만약 랜덤 이동 쓰레드가 동작 중이면 중지
    if (m_bRandomThreadRunning && m_pRandomThread != nullptr)
    {
        m_bRandomThreadRunning = false;
        WaitForSingleObject(m_pRandomThread->m_hThread, INFINITE);
        m_pRandomThread = nullptr;
    }
    EnterCriticalSection(&m_csPoints);
    m_nClickCount = 0;
    for (int i = 0; i < 3; i++)
        m_bPointSet[i] = false;
    LeaveCriticalSection(&m_csPoints);
    UpdatePointCoordinatesUI();
    Invalidate();
}

//
// [랜덤 이동] 버튼: 별도 쓰레드에서 0.5초 간격, 총 10회 랜덤 이동
//
void CMfcCircleDlg::OnBnClickedRandom()
{
    if (m_nClickCount < 3)
        return; // 아직 세 점이 없으면 무시
    if (m_bRandomThreadRunning)
        return; // 이미 실행 중

    m_bRandomThreadRunning = true;
    m_pRandomThread = AfxBeginThread(RandomMoveThread, this);
}

// 랜덤 이동 쓰레드 함수 (메인 UI 프리징 없이 수행)
UINT CMfcCircleDlg::RandomMoveThread(LPVOID pParam)
{
    CMfcCircleDlg* pDlg = reinterpret_cast<CMfcCircleDlg*>(pParam);
    if (!pDlg)
        return 0;

    RECT rc;
    ::GetClientRect(pDlg->m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    for (int i = 0; i < 10 && pDlg->m_bRandomThreadRunning; i++)
    {
        EnterCriticalSection(&pDlg->m_csPoints);
        for (int j = 0; j < 3; j++)
        {
            // m_nPointRadius를 고려하여 화면 내에 위치하도록
            pDlg->m_points[j].x = rand() % (width - 2 * pDlg->m_nPointRadius) + pDlg->m_nPointRadius;
            pDlg->m_points[j].y = rand() % (height - 2 * pDlg->m_nPointRadius) + pDlg->m_nPointRadius;
        }
        LeaveCriticalSection(&pDlg->m_csPoints);
        // UI 갱신은 메인 스레드에서 처리하도록 메시지 전송
        pDlg->PostMessage(WM_UPDATE_DRAW, 0, 0);
        Sleep(500); // 0.5초 대기
    }
    pDlg->m_bRandomThreadRunning = false;
    return 0;
}

// WM_UPDATE_DRAW 메시지 처리: UI 갱신
LRESULT CMfcCircleDlg::OnUpdateDraw(WPARAM wParam, LPARAM lParam)
{
    UpdatePointCoordinatesUI();
    Invalidate();
    return 0;
}

// 클릭 지점 원의 반지름 Edit 변경 시
void CMfcCircleDlg::OnEnChangePointRadius()
{
    // DDX로 대화상자 ↔ 멤버 변수를 동기화
    UpdateData(TRUE);

    // 입력값이 너무 작거나, 음수가 되지 않도록 보정(필요하다면)
    if (m_nPointRadius < 1)
        m_nPointRadius = 1;
    else if (m_nPointRadius > 300)
        m_nPointRadius = 300;

    // 다시 대화상자에 반영(필요하다면)
    UpdateData(FALSE);

    // 화면을 다시 그려서 새 반지름 반영
    UpdatePointCoordinatesUI();
    Invalidate();
}

// 정원 외곽선 두께 Edit 변경 시
void CMfcCircleDlg::OnEnChangeGardenThickness()
{
    // 1) 대화상자 ↔ 멤버 변수 동기화
    UpdateData(TRUE);

    // 2) 유효 범위 보정
    if (m_nGardenThickness < 1)
        m_nGardenThickness = 1;
    else if (m_nGardenThickness > 30)
        m_nGardenThickness = 30;

    UpdateData(FALSE);

    // 3) 화면 다시 그리기
    UpdatePointCoordinatesUI();
    Invalidate();
}

BOOL CMfcCircleDlg::OnEraseBkgnd(HDC hdc)
{
    return TRUE;
}


HCURSOR CMfcCircleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

