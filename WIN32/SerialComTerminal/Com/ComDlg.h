
// ComDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include <iostream>
#include "SerialBuffer.h"

typedef enum tagSERIAL_STATE
{
    SS_Unknown,
    SS_UnInit,
    SS_Init,
    SS_Started ,
    SS_Stopped ,

} SERIAL_STATE;

// CComDlg dialog
class CComDlg : public CDialogEx
{
    SERIAL_STATE	m_eState;
    HANDLE  m_hCommPort;
    HANDLE	m_hDataRx;
    HANDLE  m_hThreadTerm;
    HANDLE  m_hThreadStarted;
    HANDLE  m_hThread;

    BOOL m_abIsConnected;
    CSerialBuffer m_theSerialBuffer;
#define MAX_IN_BUF  512
    char inBuf[MAX_IN_BUF];
    uint16_t    inIdx;

// Construction
public:
	CComDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COM_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
    static unsigned __stdcall ThreadFn(void*pvParam);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButton1();

private:
    void            init();
    HRESULT			UnInit();
    HRESULT			Start();
    HRESULT			Stop();
    HRESULT			Read_N		(std::string& data,long alCount,long alTimeOut);
    HRESULT			Read_Upto	(std::string& data,char chTerminator ,long	* alCount,long alTimeOut);
    HRESULT			ReadAvailable(std::string& data);
    HRESULT			Write (CString data,DWORD dwSize);
    HRESULT         CanProcess();

    void            InvalidateHandle(HANDLE& hHandle );
    void	        CloseAndCleanHandle(HANDLE& hHandle) ;
    SERIAL_STATE    GetCurrentState() {return m_eState;}
    inline void		SetDataReadEvent()	{		SetEvent ( m_hDataRx );	}

public:
    afx_msg void OnBnClickedButton2();
    CString m_szSend;
    afx_msg void OnEnChangeEdit1();
    CListBox m_inData;
    afx_msg void OnBnClickedButton3();
};
