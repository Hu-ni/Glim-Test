
// MfcCircleDlg.h: 헤더 파일
//

#pragma once


// CMfcCircleDlg 대화 상자
class CMfcCircleDlg : public CDialogEx
{
// 생성입니다.
public:
	CMfcCircleDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
    ~CMfcCircleDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCCIRCLE_DIALOG };
#endif

	protected:
        HICON m_hIcon;

		virtual void DoDataExchange(CDataExchange* pDX);
		virtual BOOL OnInitDialog();
		afx_msg void OnPaint();
        HCURSOR OnQueryDragIcon();
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnBnClickedInit();
		afx_msg void OnBnClickedRandom();
        afx_msg void OnEnChangePointRadius();
        afx_msg void OnEnChangeGardenThickness();
		afx_msg LRESULT OnUpdateDraw(WPARAM wParam, LPARAM lParam);
        afx_msg BOOL OnEraseBkgnd(HDC hdc);
		DECLARE_MESSAGE_MAP()


        // --- 멤버 변수 ---
        // 클릭 지점 관련
        int   m_nClickCount;     // 0~3
        CPoint m_points[3];       // 클릭된 좌표
        bool   m_bPointSet[3];   // 각 점이 설정되었는지

        // 사용자 입력 값
        int m_nPointRadius;      // 클릭 지점 원의 반지름 (에디트 박스 IDC_EDIT_POINT_RADIUS)
        int m_nGardenThickness;  // 정원 외곽선 두께 (에디트 박스 IDC_EDIT_GARDEN_THICKNESS)

        // 드래그 관련
        bool  m_bDragging;
        int   m_nDraggedIndex;   // 드래그 중인 점의 인덱스 (0~2)
        CPoint m_ptLastDragPos;

        // 랜덤 이동 쓰레드 관련
        CWinThread* m_pRandomThread;
        bool        m_bRandomThreadRunning;

        // 임계 구역 (여러 스레드에서 m_points에 접근하므로)
        CRITICAL_SECTION m_csPoints;

        // 정원(세 점을 지나는 원) 계산 함수
        bool CalculateGardenCircle(CPoint& center, int& radius);

        // UI의 클릭 좌표 표시 업데이트
        void UpdatePointCoordinatesUI();

public:
    // 랜덤 이동 쓰레드 함수 (static 멤버)
    static UINT RandomMoveThread(LPVOID pParam);
};
