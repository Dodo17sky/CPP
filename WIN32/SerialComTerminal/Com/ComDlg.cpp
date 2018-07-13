
// ComDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Com.h"
#include "ComDlg.h"
#include "afxdialogex.h"

#include <Windows.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CComDlg dialog

#define add_Info(x) LPCTSTR lpszUnicode =  TEXT(x); this->m_inData.AddString(lpszUnicode)

CComDlg::CComDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_COM_DIALOG, pParent)
    , m_szSend(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CComDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT2, m_szSend);
    DDX_Control(pDX, IDC_LIST1, m_inData);
}

BEGIN_MESSAGE_MAP(CComDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CComDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CComDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON3, &CComDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CComDlg message handlers

BOOL CComDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    InvalidateHandle( m_hThreadTerm );
    InvalidateHandle( m_hThread	);
    InvalidateHandle( m_hThreadStarted );
    InvalidateHandle( m_hCommPort );
    InvalidateHandle( m_hDataRx );

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CComDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CComDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CComDlg::init()
{
    memset(inBuf, 0, MAX_IN_BUF);
    inIdx = 0;

    m_hDataRx  = CreateEvent(0,0,0,0);

    m_hCommPort = CreateFile(TEXT("COM4"),
        GENERIC_READ|GENERIC_WRITE,//access ( read and write)
        0,    //(share) 0:cannot share the COM port                        
        0,    //security  (None)                
        OPEN_EXISTING,// creation : open_existing
        FILE_FLAG_OVERLAPPED,// we want overlapped operation
        0// no templates file for COM port...
    );

    if ( m_hCommPort == INVALID_HANDLE_VALUE ) {
        std::cerr << "Failed to open COM Port Reason: " << GetLastError() << std::endl;
        add_Info( "Failed to open COM Port Reason: " );
        return;
    }
    else {
        std::cout << "COM Port is open" << std::endl;
        add_Info( "COM Port is open" );
    }

    if (! SetCommMask(m_hCommPort,EV_RXCHAR|EV_TXEMPTY) ) {
        std::cerr << "CSerialCommHelper : Failed to Set Comm Mask Reason: " << GetLastError() << std::endl;
        add_Info( "CSerialCommHelper : Failed to Set Comm Mask Reason: " );
    }

    //now we need to set baud rate etc,
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!::GetCommState (m_hCommPort,&dcb)) {
        std::cerr << "CSerialCommHelper : Failed to Get Comm State Reason: %d" << GetLastError() << std::endl;
        add_Info( "CSerialCommHelper : Failed to Get Comm State Reason:" );
    }

    dcb.BaudRate	= CBR_9600;
    dcb.ByteSize	= DATABITS_8;
    dcb.Parity		= NOPARITY;
    dcb.StopBits	= ONESTOPBIT;
    dcb.fDsrSensitivity = 0;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fOutxDsrFlow = 0;

    if (!::SetCommState (m_hCommPort,&dcb)) {
        std::cout << "CSerialCommHelper : Failed to Set Comm State Reason: %d" << GetLastError() << std::endl;
        add_Info("CSerialCommHelper : Failed to Set Comm State Reason:");
    }

    //now set the timeouts ( we control the timeout overselves using WaitForXXX()
    COMMTIMEOUTS timeouts;

    timeouts.ReadIntervalTimeout		    = MAXDWORD; 
    timeouts.ReadTotalTimeoutMultiplier		= 0;
    timeouts.ReadTotalTimeoutConstant		= 0;
    timeouts.WriteTotalTimeoutMultiplier	= 0;
    timeouts.WriteTotalTimeoutConstant		= 0;

    if (!SetCommTimeouts(m_hCommPort, &timeouts)) {
        std::cout << "CSerialCommHelper :  Error setting time-outs " << GetLastError() << std::endl;
        add_Info("CSerialCommHelper :  Error setting time-outs ");
    }

    //create thread terminator event...
    m_hThreadTerm = CreateEvent(0,0,0,0);
    m_hThreadStarted = CreateEvent(0,0,0,0);

    m_hThread = (HANDLE)_beginthreadex(0,0,CComDlg::ThreadFn,(void*)this,0,0 );

    DWORD dwWait = WaitForSingleObject ( m_hThreadStarted , INFINITE );

    ASSERT ( dwWait ==	WAIT_OBJECT_0 );

    CloseHandle(m_hThreadStarted);
    InvalidateHandle(m_hThreadStarted );

    LPCTSTR lpszUnicode = TEXT("init() end");
    this->m_inData.AddString(lpszUnicode);

    m_abIsConnected = true;
}

HRESULT CComDlg::UnInit()
{
    return E_NOTIMPL;
}

HRESULT CComDlg::Start()
{
    m_eState = SS_Started;
    return S_OK;
}

HRESULT CComDlg::Stop()
{
    m_eState = SS_Stopped;
    return S_OK;
}

HRESULT CComDlg::Write(CString data, DWORD dwSize)
{
    HRESULT hr = CanProcess();
    if ( FAILED(hr)) return hr;
    int iRet = 0 ;
    OVERLAPPED ov;

    data.Append("\n");
    dwSize += 1;

    memset(&ov,0,sizeof(ov));
    ov.hEvent = CreateEvent( 0,true,0,0);
    DWORD dwBytesWritten = 0;
    
    do
    {
        iRet = WriteFile (m_hCommPort,data,dwSize,&dwBytesWritten  ,&ov);
        if ( iRet == 0 )
        {
            WaitForSingleObject(ov.hEvent ,INFINITE);
        }

    }while ( ov.InternalHigh != dwSize ) ;

    CloseHandle(ov.hEvent);

    return S_OK;
}

void CComDlg::InvalidateHandle(HANDLE & hHandle)
{
    hHandle = INVALID_HANDLE_VALUE;
}

unsigned CComDlg::ThreadFn(void * pvParam)
{
    CComDlg* apThis = (CComDlg*) pvParam ;
    bool abContinue = true;
    DWORD dwEventMask=0;

    OVERLAPPED ov;
    memset(&ov,0,sizeof(ov));
    ov.hEvent = CreateEvent( 0,true,0,0);
    // 0: ThreadTerm event
    // 1: serial Rx/Tx event
    HANDLE arHandles[2];
    arHandles[0] = apThis->m_hThreadTerm;

    DWORD dwWait;

    SetEvent(apThis-> m_hThreadStarted);

    while (abContinue) {
        // Wait to Rx or Tx on serial port
        BOOL abRet = ::WaitCommEvent(apThis->m_hCommPort, &dwEventMask, &ov);

        if (!abRet) {
            // neither Rx/Tx operation - means that there are no Rx/Tx requests
            ASSERT(GetLastError() == ERROR_IO_PENDING);
        }

        arHandles[1] = ov.hEvent;

        dwWait = WaitForMultipleObjects(2, arHandles, FALSE, INFINITE);
        switch (dwWait) {
            case WAIT_OBJECT_0:
            {
                // End of Rx/Tx requests. Closing the serial port
                _endthreadex(1);
            }
            break;
            case WAIT_OBJECT_0 + 1:
            {
                DWORD dwMask;
                if (GetCommMask(apThis->m_hCommPort, &dwMask))
                {
                    if ( dwMask == EV_TXEMPTY)
                    {
                        LPCTSTR lpszUnicode = TEXT("Data sent");
                        apThis->m_inData.AddString(lpszUnicode);

                        ResetEvent(ov.hEvent);
                        continue;
                    }

                }

                //read data here...
                int iAccum = 0;
                apThis->m_theSerialBuffer.LockBuffer();

                try 
                {
                    std::string szDebug;
                    BOOL abRet = false;

                    DWORD dwBytesRead = 0;
                    OVERLAPPED ovRead;
                    memset(&ovRead,0,sizeof(ovRead));
                    ovRead.hEvent = CreateEvent( 0,true,0,0);

                    do
                    {
                        ResetEvent( ovRead.hEvent  );
                        char szTmp[1];
                        int iSize  = sizeof ( szTmp );
                        memset(szTmp,0,sizeof szTmp);
                        abRet = ::ReadFile(apThis->m_hCommPort,szTmp,sizeof(szTmp),&dwBytesRead,&ovRead);
                        if (!abRet ) 
                        {
                            abContinue = FALSE;
                            break;
                        }
                        if ( dwBytesRead > 0 )
                        {
                            if (apThis->inIdx == (MAX_IN_BUF - 1)) {
                                memset(apThis->inBuf, 0, MAX_IN_BUF);
                                apThis->inIdx = 0;
                                apThis->inBuf[apThis->inIdx] = szTmp[0];
                                continue;
                            }

                            apThis->inBuf[apThis->inIdx] = szTmp[0];
                            apThis->inIdx += 1;

                            if (szTmp[0] == '\n') {
                                std::string s(apThis->inBuf);
                                LPCTSTR lpszUnicode = s.c_str();
                                apThis->m_inData.AddString(lpszUnicode);
                                
                                memset(apThis->inBuf, 0, MAX_IN_BUF);
                                apThis->inIdx = 0;
                            }

                        }
                    }while (0);// dwBytesRead > 0 );
                    CloseHandle(ovRead.hEvent );
                }
                catch(...)
                {
                    ASSERT(0);
                }

                ResetEvent ( ov.hEvent );
            }
            break;
        } // switch
    }

    return 0;
}

HRESULT  CComDlg::CanProcess ()
{

    switch ( m_eState  ) 
    {
    case SS_Unknown	:ASSERT(0);return E_FAIL;
    case SS_UnInit	:return E_FAIL;
    case SS_Started :return S_OK;
    case SS_Init		:
    case SS_Stopped :
        return E_FAIL;
    default:ASSERT(0);	

    }	
    return E_FAIL;
}

void CComDlg::OnBnClickedButton1()
{
    this->init();
    this->Start();
}

void CComDlg::OnBnClickedButton2()
{
    UpdateData();
    this->Write(m_szSend, m_szSend.GetLength () );
}

void CComDlg::OnEnChangeEdit1()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


void CComDlg::OnBnClickedButton3()
{
    this->Stop();
}
