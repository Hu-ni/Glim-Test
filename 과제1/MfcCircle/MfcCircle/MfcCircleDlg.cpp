
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
    m_bRandomThreadRunning(false),
    m_nImageBpp(8)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    for (int i = 0; i < 3; i++) {
        m_bPointSet[i] = false;
        m_points[i] = CPoint(0, 0);
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
    if (m_pRandomThread) {
        m_bRandomThreadRunning = false;
        WaitForSingleObject(m_pRandomThread->m_hThread, INFINITE);
    }
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

    // 클라이언트 영역을 기준으로 이미지 크기 설정 (대화상자 전체를 그림 영역으로 사용)
    CRect rect;
    GetClientRect(&rect);
    m_nImageWidth = rect.Width();
    m_nImageHeight = rect.Height();

    // m_image 생성: m_nImageWidth × m_nImageHeight, 8비트 그레이스케일, top-down DIB (-m_nImageHeight)
    m_image.Create(m_nImageWidth, -m_nImageHeight, m_nImageBpp);

    // 그레이스케일 컬러 테이블 설정
    if (m_nImageBpp == 8) {
        RGBQUAD rgb[256];
        for (int i = 0; i < 256; i++) {
            rgb[i].rgbRed = rgb[i].rgbGreen = rgb[i].rgbBlue = i;
            rgb[i].rgbReserved = 0;
        }
        m_image.SetColorTable(0, 256, rgb);
    }

    // 이미지 전체를 흰색(0xff)으로 채웁니다.
    int nPitch = m_image.GetPitch();
    unsigned char* bits = (unsigned char*)m_image.GetBits();
    memset(bits, 0xff, nPitch * m_nImageHeight);

    return TRUE;
}

HCURSOR CMfcCircleDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

//
// OnPaint: CDC 대신 HDC와 순수 GDI 함수를 사용하여 그리기
//
void CMfcCircleDlg::OnPaint()
{
    CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트
    if (IsIconic())
    {
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        // m_image를 대화상자에 그리기
        UpdateDisplay();
        CDialogEx::OnPaint();
    }
}

// 세 점을 지나는 원(정원) 계산
bool CMfcCircleDlg::CalculateGardenCircle(const CPoint& p1, const CPoint& p2, const CPoint& p3, CPoint& center, double& radius)
{
    double x1 = p1.x, y1 = p1.y;
    double x2 = p2.x, y2 = p2.y;
    double x3 = p3.x, y3 = p3.y;

    double denominator = 2.0 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
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
    radius = sqrt((x1 - a) * (x1 - a) + (y1 - b) * (y1 - b));
    return true;
}

void CMfcCircleDlg::DrawGardenCircle(CImage& image, const CPoint& center, double radius, int thickness, COLORREF color)
{
    int nWidth = image.GetWidth();
    int nHeight = image.GetHeight();
    int nPitch = image.GetPitch();
    unsigned char* bits = (unsigned char*)image.GetBits();

    // 외곽선 두께에 따른 내부/외부 경계 계산
    double rOuter = radius + thickness / 2.0;
    double rInner = (radius - thickness / 2.0 > 0) ? radius - thickness / 2.0 : 0;
    double rOuterSq = rOuter * rOuter;
    double rInnerSq = rInner * rInner;

    int x0 = max(0, center.x - (int)rOuter);
    int x1 = min(nWidth - 1, center.x + (int)rOuter);
    int y0 = max(0, center.y - (int)rOuter);
    int y1 = min(nHeight - 1, center.y + (int)rOuter);

    // 8비트 그레이스케일 이미지로 가정하므로, 색상은 회색 값(R=G=B)
    unsigned char gray = (unsigned char)GetRValue(color);

    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++)
        {
            double dx = x - center.x;
            double dy = y - center.y;
            double distSq = dx * dx + dy * dy;
            if (distSq <= rOuterSq && distSq >= rInnerSq)
            {
                bits[y * nPitch + x] = gray;
            }
        }
    }
}

void CMfcCircleDlg::DrawClickedPoints(CImage& image, int pointRadius, unsigned char gray)
{
    int nWidth = image.GetWidth();
    int nHeight = image.GetHeight();
    int nPitch = image.GetPitch();
    unsigned char* bits = (unsigned char*)image.GetBits();

    for (int idx = 0; idx < m_nClickCount; idx++) {
        CPoint pt = m_points[idx];

        int x0 = max(0, pt.x - pointRadius);
        int x1 = min(nWidth - 1, pt.x + pointRadius);
        int y0 = max(0, pt.y - pointRadius);
        int y1 = min(nHeight - 1, pt.y + pointRadius);
        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                double dx = x - pt.x;
                double dy = y - pt.y;
                if (dx * dx + dy * dy <= pointRadius * pointRadius) {
                    bits[y * nPitch + x] = gray;
                }
            }
        }
    }
}

void CMfcCircleDlg::RedrawImage()
{
    // m_image는 OnInitDialog에서 생성되었으므로 여기서는 내용만 초기화
    int nPitch = m_image.GetPitch();
    unsigned char* bits = (unsigned char*)m_image.GetBits();
    memset(bits, 0xff, nPitch * m_nImageHeight);

    // 클릭된 점 원(마커) 그리기 (검정색: 0)
    DrawClickedPoints(m_image,  m_nPointRadius, 0);

    // 세 점 모두 입력되었으면 정원(세 점을 지나는 원) 그리기
    if (m_nClickCount == 3) {
        CPoint center;
        double radius;
        if (CalculateGardenCircle(m_points[0], m_points[1], m_points[2], center, radius)) {
            DrawGardenCircle(m_image, center, radius, m_nGardenThickness, 0);
        }
    }
}

// 화면 갱신: m_image를 (0,0)에 그립니다.
void CMfcCircleDlg::UpdateDisplay()
{
    CClientDC dc(this);
    m_image.Draw(dc.GetSafeHdc(), 0, 0);
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
    EnterCriticalSection(&m_csPoints);
    if (m_nClickCount < 3)
    {
        m_points[m_nClickCount] = point;
        m_bPointSet[m_nClickCount] = true;
        m_nClickCount++;

        UpdatePointCoordinatesUI();
        RedrawImage();
        UpdateDisplay();
    }
    else
    {
        // 세 점이 모두 존재하면 드래그 모드로 전환: 클릭한 위치가 기존 점 내에 있는지 확인
        for (int i = 0; i < 3; i++)
        {
            int dx = point.x - m_points[i].x;
            int dy = point.y - m_points[i].y;
            if (dx * dx + dy * dy <= m_nPointRadius * m_nPointRadius)
            {
                m_bDragging = true;
                m_nDraggedIndex = i;
                m_ptLastDragPos = point;
                break;
            }
        }
    }
    LeaveCriticalSection(&m_csPoints);

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
        RedrawImage();
        UpdateDisplay();

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
    RedrawImage();
    UpdateDisplay();
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

    const int iterations = 10;
    const int sleeptime = 500;

    RECT rc;
    ::GetClientRect(pDlg->m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    for (int i = 0; i < iterations && pDlg->m_bRandomThreadRunning; i++)
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
        Sleep(sleeptime); // 0.5초 대기
    }
    pDlg->m_bRandomThreadRunning = false;
    return 0;
}

// WM_UPDATE_DRAW 메시지 처리: UI 갱신
LRESULT CMfcCircleDlg::OnUpdateDraw(WPARAM wParam, LPARAM lParam)
{
    UpdatePointCoordinatesUI();
    RedrawImage();
    UpdateDisplay();
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
    RedrawImage();
    UpdateDisplay();
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
    RedrawImage();
    UpdateDisplay();
}
