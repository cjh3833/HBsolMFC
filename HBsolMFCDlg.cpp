// HBsolMFCDlg.cpp: 구현 파일
// 123123

#pragma once
#include "pch.h"
#include "framework.h"
#include "HBsolMFC.h"
#include "HBsolMFCDlg.h"
#include "afxdialogex.h"
#include <string>
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <pylon/PylonBase.h>
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>
#include "MarkDetector.h";

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace cv;
using namespace Pylon;
using namespace std;

// ------------------------------
// ---------- 전역변수 ----------

static const uint32_t c_countOfImagesToGrab = 100;
Mat camImage; // 해당 변수 사용시 임계구역 선언 필수!
CRITICAL_SECTION cs;
CImageFormatConverter fc;
CPylonImage pylonImage;
CStatic m_temple;

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

    // 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

 // 구현입니다.
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHBsolMFCDlg 대화 상자

CHBsolMFCDlg::CHBsolMFCDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_HBSOLMFC_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CHBsolMFCDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CAM_VIEW, m_CamView);
    DDX_Control(pDX, IDC_CAM_TEM, m_temple);
    DDX_Control(pDX, IDC_EDIT_IP, m_IPname);
    DDX_Control(pDX, IDC_EDIT_PORT, m_Portname);
}

BEGIN_MESSAGE_MAP(CHBsolMFCDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_CAM_START, &CHBsolMFCDlg::OnBnClickedCamStart)
    ON_BN_CLICKED(IDC_CAM_STOP, &CHBsolMFCDlg::OnBnClickedCamStop)
    ON_BN_CLICKED(IDC_TEMPLE, &CHBsolMFCDlg::OnBnClickedTemple)
    ON_BN_CLICKED(IDC_SERVER, &CHBsolMFCDlg::OnBnClickedServer)
    ON_COMMAND(ID_LIGHTCONTROL_1, &CHBsolMFCDlg::OnLightcontrol1)
    ON_COMMAND(ID_TEMPLATE_1, &CHBsolMFCDlg::OnTemplate1)
    ON_COMMAND(ID_TEMPLATE_2, &CHBsolMFCDlg::OnTemplate2)
    ON_COMMAND(ID_TEMPLATE_3, &CHBsolMFCDlg::OnTemplate3)
    ON_WM_CTLCOLOR()
    ON_COMMAND(ID_CAMERA_CAMSTART, &CHBsolMFCDlg::OnCameraCamstart)
    ON_COMMAND(ID_CAMERA_CAMSTOP, &CHBsolMFCDlg::OnCameraCamstop)
END_MESSAGE_MAP()


// CHBsolMFCDlg 메시지 처리기

BOOL CHBsolMFCDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

    // IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
    //  프레임워크가 이 작업을 자동으로 수행합니다.
    SetIcon(m_hIcon, TRUE);         // 큰 아이콘을 설정합니다.
    SetIcon(m_hIcon, FALSE);      // 작은 아이콘을 설정합니다.

    // TODO: 여기에 추가 초기화 작업을 추가합니다.
    PylonInitialize();
    InitializeCriticalSection(&cs);
    fc.OutputPixelFormat = PixelType_RGB8packed;

    // Get the transport layer factory.
    CTlFactory& tlFactory = CTlFactory::GetInstance();

    // Get all attached devices and exit application if no device is found.
    DeviceInfoList_t devices;
    if (tlFactory.EnumerateDevices(devices) == 0) {
        MessageBox(L"Basler Camera가 연결되지 않았습니다.");
    }
    else {
        // create device
        //IPylonDevice *pDevice = TlFactory.CreateDevice(lstDevices[0]);
        // Create an instant camera object with the camera device found first.
        m_pCamera = new CInstantCamera(CTlFactory::GetInstance().CreateFirstDevice());

        try
        {
            // 카메라 파리미터 설정하기
            m_pCamera->Open();

            INodeMap& nodemap = m_pCamera->GetNodeMap();
            // Get the integer nodes describing the AOI.
            CIntegerPtr offsetX(nodemap.GetNode("OffsetX"));
            CIntegerPtr offsetY(nodemap.GetNode("OffsetY"));
            CIntegerPtr width(nodemap.GetNode("Width"));
            CIntegerPtr height(nodemap.GetNode("Height"));

            // GenApi has some convenience predicates to check this easily.
            int new_width = 1024;
            int new_height = 624;   // 768시 error 발생
            if (IsWritable(width))   width->SetValue(new_width);
            if (IsWritable(height))  height->SetValue(new_height);
            if (IsWritable(offsetX)) offsetX->SetValue(new_width / 2);
            if (IsWritable(offsetY)) offsetY->SetValue(new_height / 2);

            // 카메라 정보 출력
            CString info;
            info.Format(_T("Found camera!\n<%s>\nSize X: %d\nSize Y: %d"),
                CString(m_pCamera->GetDeviceInfo().GetModelName()),
                width->GetValue(), height->GetValue());
            AfxMessageBox(info);

            m_pCamera->Close();
        }
        catch (GenICam::GenericException& e) //Error Handling
        {
            // Error handling
            CString strTrace;
            strTrace.Format(_T("Open_Camera - GenericException : %s\n"), (CString)e.GetDescription());
            AfxMessageBox(strTrace);
            return FALSE;
        }
    }
    return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CHBsolMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CHBsolMFCDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // 아이콘을 그립니다.
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CHBsolMFCDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// --------------------------------------
// ---------- 사용자 정의 함수 ----------

UINT ThreadImageCaptureFunc(LPVOID param)
{
    CHBsolMFCDlg* pDlg = (CHBsolMFCDlg*)param;
    if (pDlg->m_pCamera == NULL) return 0;

    while (pDlg->m_bThreadFlag) {
        // m_pCamera->StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        try {

            if (pDlg->m_pCamera->IsGrabbing()) {
                // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
                pDlg->m_pCamera->RetrieveResult(5000, pDlg->m_ptrGrabResult, TimeoutHandling_ThrowException);

                // Image grabbed successfully?
                if (pDlg->m_ptrGrabResult->GrabSucceeded()) {

                    // Color로 포맷을 변경할 경우
                    //CImageFormatConverter fc;
                    //fc.OutputPixelFormat = PixelType_RGB8packed;
                    //CPylonImage pylonImage;
                    fc.Convert(pylonImage, pDlg->m_ptrGrabResult);

                    // -------------------------------------
                    // ---------- CRITICAL SECTION ---------

                    EnterCriticalSection(&cs);
                    camImage = Mat(pDlg->m_ptrGrabResult->GetHeight(), pDlg->m_ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());

                    CRect rect;
                    CDC* pDC = pDlg->m_CamView.GetDC();           // 출력한 부분의 DC 얻기
                    pDlg->m_CamView.GetClientRect(rect);               // 출력할 영역 얻기
                    DisplayImage(pDC, rect, camImage);  // 카메라에서 읽어들인 영상을 화면에 그리기
                    LeaveCriticalSection(&cs);

                    // ---------- CRITICAL SECTION ---------
                    // -------------------------------------

                    pDlg->ReleaseDC(pDC);

                }
                else {
                    CString str;
                    str.Format(_T("Error: %d %d"), pDlg->m_ptrGrabResult->GetErrorCode(), pDlg->m_ptrGrabResult->GetErrorDescription());
                    AfxMessageBox(str);
                    pDlg->m_bThreadFlag = false;
                }
            }
        }
        catch (GenICam::GenericException& e) {
            // Error handling.
            cerr << "An exception occurred." << e.GetDescription() << endl;
        }
    }

    return 0;
}
void FillBitmapInfo(BITMAPINFO* bmi, int width, int height, int bpp, int origin)
{
    assert(bmi && width >= 0 && height >= 0 && (bpp == 8 || bpp == 24 || bpp == 32));
    BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);
    memset(bmih, 0, sizeof(*bmih));
    bmih->biSize = sizeof(BITMAPINFOHEADER);
    bmih->biWidth = width;
    bmih->biHeight = origin ? abs(height) : -abs(height);
    bmih->biPlanes = 1;
    bmih->biBitCount = (unsigned short)bpp;
    bmih->biCompression = BI_RGB;
    if (bpp == 8) {
        RGBQUAD* palette = bmi->bmiColors;
        for (int i = 0; i < 256; i++) {
            palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
            palette[i].rgbReserved = 0;
        }
    }
}

void DisplayImage(CDC* pDC, CRect rect, Mat& srcimg)
{
    Mat img;
    int width = ((int)(rect.Width() / 4)) * 4; // 4byte 단위조정해야 영상이 기울어지지 않는다.
    resize(srcimg, img, Size(width, rect.Height()));
    uchar buffer[sizeof(BITMAPINFOHEADER) * 1024];
    BITMAPINFO* bmi = (BITMAPINFO*)buffer;

    int bmp_w = img.cols;
    int bmp_h = img.rows;
    int depth = img.depth();
    int channels = img.channels();
    int bpp = 8 * channels;

    FillBitmapInfo(bmi, bmp_w, bmp_h, bpp, 0);

    int from_x = MIN(0, bmp_w - 1);
    int from_y = MIN(0, bmp_h - 1);
    int sw = MAX(MIN(bmp_w - from_x, rect.Width()), 0);
    int sh = MAX(MIN(bmp_h - from_y, rect.Height()), 0);

    SetDIBitsToDevice(pDC->m_hDC, rect.left, rect.top, sw, sh, from_x, from_y, 0, sh, img.data + from_y * img.step, bmi, 0);
    img.release();
}
void CHBsolMFCDlg::DisplayTemImage(cv::Mat& _targetMat)
{   //5.09
    // CDC 포인터 및 MFC 이미지 객체 포인터 초기화
    CDC* pDC;
    CImage* mfcImg = nullptr;

    // m_temple 컨트롤의 디바이스 컨텍스트를 가져옴
    pDC = m_temple.GetDC();

    // 입력 이미지를 확대하여 tempImage에 저장
    cv::Mat tempImage;
    cv::resize(_targetMat, tempImage, Size(_targetMat.cols * 1, _targetMat.rows * 1));//0.98기준사진

    // 비트맵 정보 구조체 초기화
    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biBitCount = 24;
    bitmapInfo.bmiHeader.biWidth = tempImage.cols;
    bitmapInfo.bmiHeader.biHeight = tempImage.rows;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    bitmapInfo.bmiHeader.biClrImportant = 0;
    bitmapInfo.bmiHeader.biClrUsed = 0;
    bitmapInfo.bmiHeader.biSizeImage = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter = 0;



    // 입력 이미지의 채널 수에 따라 분기
    if (_targetMat.channels() == 3)
    {   // 채널 수가 3인 경우, 입력 이미지를 그대로 사용하여 24비트 이미지 생성
        //tempImage = _targetMat.clone();
        mfcImg = new CImage();
        mfcImg->Create(tempImage.cols, tempImage.rows, 24);
    }

    else if (_targetMat.channels() == 1)
    {   // 채널 수가 1인 경우, 흑백 이미지를 컬러 이미지로 변환하여 24비트 이미지 생성
        cvtColor(tempImage, tempImage, COLOR_GRAY2BGR);
        mfcImg = new CImage();
        mfcImg->Create(tempImage.cols, tempImage.rows, 24);
    }
    else if (_targetMat.channels() == 4)
    {
        // 채널 수가 4인 경우, 32비트 이미지 생성
        //tempImage = _targetMat.clone();
        bitmapInfo.bmiHeader.biBitCount = 32;
        mfcImg = new CImage();
        mfcImg->Create(tempImage.cols, tempImage.rows, 32);
    }

    // 이미지를 수직 방향으로 뒤집음
    cv::flip(tempImage, tempImage, 0);

    // tempImage를 mfcImg에 그림
    ::StretchDIBits(mfcImg->GetDC(), 0, 0, tempImage.cols, tempImage.rows,
        0, 0, tempImage.cols, tempImage.rows, tempImage.data, &bitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);

    // mfcImg를 m_temple 윈도우에 그림
    mfcImg->BitBlt(::GetDC(m_temple.m_hWnd), 0, 0);

    // 메모리 해제
    if (mfcImg)
    {
        mfcImg->ReleaseDC();
        delete mfcImg; mfcImg = nullptr;
    }
    tempImage.release();
    ReleaseDC(pDC);
}


void CHBsolMFCDlg::OnDestroy()
{
    

    // TODO: 여기에 메시지 처리기 코드를 추가합니다.
    // Releases all pylon resources. 
    PylonTerminate();
    DeleteCriticalSection(&cs);

    m_pCamera->StopGrabbing();
    m_bThreadFlag = FALSE;

    if (m_pCamera != NULL) delete m_pCamera;
}

void CHBsolMFCDlg::OnBnClickedCamStart()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    if (m_pCamera == NULL) {
        MessageBox(L"Basler Camera를 연결 후 다시 실행시켜주세요.");
        return;
    }
    m_pCamera->StartGrabbing( /*c_countOfImagesToGrab*/);
    m_bThreadFlag = TRUE;
    CWinThread* pThread = ::AfxBeginThread(ThreadImageCaptureFunc, this);

}

void CHBsolMFCDlg::OnBnClickedCamStop()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

    if (m_pCamera == NULL) {
        MessageBox(L"Basler Camera를 연결 후 다시 실행시켜주세요.");
        return;
    }
    m_pCamera->StopGrabbing();
    m_bThreadFlag = FALSE;          // 쓰레드 정지 시킴

}

void CHBsolMFCDlg::OnBnClickedTemple()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    if (m_pCamera == NULL) {
        MessageBox(L"Basler Camera를 연결 후 다시 실행시켜주세요.");
        return;
    }

    // -------------------------------------
    // ---------- CRITICAL SECTION ---------

    EnterCriticalSection(&cs);
    Mat frame = camImage;
    LeaveCriticalSection(&cs);

    // ---------- CRITICAL SECTION ---------
    // -------------------------------------

    cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
    cv::imshow("frame", frame);

    Mat templ = imread("templ.png", cv::IMREAD_GRAYSCALE);
    if (templ.empty()) {
        MessageBox(L"Template 이미지가 없습니다");
        return;
    }

    MarkDetector md;
    Mat templ_out = md.trainEdge(templ);
    cv::imshow("templ", templ_out);

    Mat frame_out = md.matchTemplateByEdge(frame);
    cv::imshow("result", frame_out);

    DisplayTemImage(frame_out);

    

    

}

void CHBsolMFCDlg::OnBnClickedServer()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    CString strIP = _T("");     //문자열을 저장하기 위해 CString 클래스의 strIP 객체 생성
    CString strPort = _T("");     //문자열을 저장하기 위해 CString 클래스의 strPort 객체 생성
    CString strIPM = _T("입력된 IP : ");     //문자열을 저장하기 위해 CString 클래스의 strIP 객체 생성
    CString strPortM = _T("  입력된 Port : ");     //문자열을 저장하기 위해 CString 클래스의 strPort 객체 생성
    GetDlgItemText(IDC_EDIT_IP, strIP); // IDC_EDIT1 컨트롤부터 문자열을 읽어 strIP에 저장.
    GetDlgItemText(IDC_EDIT_PORT, strPort); // IDC_EDIT1 컨트롤부터 문자열을 읽어 strPort에 저장.
    MessageBox(strIPM + strIP + strPortM + strPort, MB_OK);    //메세지알림으로 str을 출력하기

}


void CHBsolMFCDlg::OnLightcontrol1()
{   //툴바 lighcontrol 1번째줄 메뉴
   // TODO: 여기에 명령 처리기 코드를 추가합니다.
}


void CHBsolMFCDlg::OnTemplate1()
{   //툴바 template 1번째줄 메뉴
   // TODO: 여기에 명령 처리기 코드를 추가합니다.
    Mat src = imread("templ.png");
    DisplayTemImage(src);

}


void CHBsolMFCDlg::OnTemplate2()
{   //툴바 template 2번째줄 메뉴
   // TODO: 여기에 명령 처리기 코드를 추가합니다.
}


void CHBsolMFCDlg::OnTemplate3()
{   //툴바 template 3번째줄 메뉴
   // TODO: 여기에 명령 처리기 코드를 추가합니다.
}


HBRUSH CHBsolMFCDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // TODO:  여기서 DC의 특성을 변경합니다.
    switch (pWnd->GetDlgCtrlID())
    {
    case IDC_STATIC_LOG:
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(0, 0, 0));
        break;
    }
    // TODO:  기본값이 적당하지 않으면 다른 브러시를 반환합니다.
    return hbr;
}

void CHBsolMFCDlg::OnCameraCamstart()
{   //Cam Start Menu
    // TODO: 여기에 명령 처리기 코드를 추가합니다.
    if (m_pCamera == NULL) {
        MessageBox(L"Basler Camera를 연결 후 다시 실행시켜주세요.");
        return;
    }
    m_pCamera->StartGrabbing( /*c_countOfImagesToGrab*/);
    m_bThreadFlag = TRUE;
    CWinThread* pThread = ::AfxBeginThread(ThreadImageCaptureFunc, this);
}


void CHBsolMFCDlg::OnCameraCamstop()
{
    // TODO: 여기에 명령 처리기 코드를 추가합니다.
    if (m_pCamera == NULL) {
        MessageBox(L"Basler Camera를 연결 후 다시 실행시켜주세요.");
        return;
    }
    m_pCamera->StopGrabbing();
    m_bThreadFlag = FALSE;          // 쓰레드 정지 시킴
}
