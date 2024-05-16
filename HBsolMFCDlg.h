
// HBsolMFCDlg.h: 헤더 파일
//

#pragma once
// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/PylonBase.h>

// OpenCV
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>


using namespace Pylon;
using namespace GenApi;
using namespace cv;


UINT ThreadImageCaptureFunc(LPVOID param);  // 쓰레드 함수
UINT ThreadImageCaptureFuncMFC(LPVOID param);  // 쓰레드 함수
void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin);
void DisplayImage(CDC* pDC, CRect rect, Mat& srcimg);
void CreateBitmapInfo(int w, int h, int bpp); // Bitmap 정보를 생성하는 함수.
void DrawImage(); // 그리는 작업을 수행하는 함수.
// CHBsolMFCDlg 대화 상자
class CHBsolMFCDlg : public CDialogEx
{
// 생성입니다.
public:
	CHBsolMFCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HBSOLMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

public:
	BOOL m_bThreadFlag;                 // 쓰레드 루프 돌기
	BOOL m_bThreadFlag2;                 // 쓰레드 루프 돌기
	CInstantCamera* m_pCamera;          // Basler Camera
	CGrabResultPtr m_ptrGrabResult;     // grab result data 받기

// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	
public:
	CStatic m_CamView;
	CStatic m_temple;
	afx_msg
	
	void OnDestroy();
	void OnBnClickedCamStart();
	afx_msg void OnBnClickedCamStop();
	afx_msg void OnBnClickedTemple();
	
	afx_msg void OnBnClickedServer();
	CEdit m_IPname;
	CEdit m_Portname;
	afx_msg void OnLightcontrol1();
	afx_msg void OnTemplate1();
	afx_msg void OnTemplate2();
	afx_msg void OnTemplate3();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	Mat m_matImage;
	BITMAPINFO* m_pBitmapInfo;

	void CHBsolMFCDlg::DisplayTemImage(cv::Mat& _targetMat);
	void CHBsolMFCDlg::DisplayMatchImage(cv::Mat& _targetMat);
	void CreateBitmapInfo(int w, int h, int bpp);
	void DrawImage();
	afx_msg void OnCameraCamstart();
	afx_msg void OnCameraCamstop();
	CStatic m_result;
};
