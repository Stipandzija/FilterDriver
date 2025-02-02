// MFCApplication2Dlg.h
#include "SettingsDialog.h"
#include <deque>
#include <vector>
#include <thread>

using namespace std;

struct SERIAL_CONFIG {
    int BaudRate;
    int WordLength;
    int StopBits;
    int Parity;
};

class CMFCApplication2Dlg : public CDialogEx {

    HANDLE devicehandle;
    CEdit EditControl;
    CString SelectedPort;
    SERIAL_CONFIG SerialConfig;
    CSettingsDialog SettingsDialog;

    HICON hIcon;
    CString DevicePath;

    CEdit EditOutput;

    BOOL isConnected = false;

    BOOL StayOnTop;
    BOOL QuitOnEscape;
    BOOL AutoCompleteEditLine;
    BOOL KeepHistory;
    BOOL ClosePortWhenInactive;

    BOOL isFalse = false;
    int currentIndex = 0;
    thread t1;

public:
    CMFCApplication2Dlg(CWnd* pParent = nullptr);
    DECLARE_MESSAGE_MAP();
    virtual BOOL OnInitDialog();

    void OnSysCommand(UINT, LPARAM);
    void OnBnClickedButton1();
    void OnBnClickedButton2();
    void OnEnChangeEdit1();
    void OnBnClickedButton5();
    void OnBnClickedSettingsButton();
    void OnBnClickedButtonConnect();
    void AppendTextToOutput(CString);
    void OnTimer(UINT_PTR);
    void CMFCApplication2Dlg::UpdateConnectButton();
    void OnBnClickedRecive();
    void OnEnChangeBox();
    static UINT ThreadFunc(LPVOID);
    void OnClose();

    virtual void DoDataExchange(CDataExchange*);
    virtual BOOL PreTranslateMessage(MSG*);
    void CMFCApplication2Dlg::OnKeyDown(UINT, UINT, UINT);


    void SendIoControl(HANDLE hDevice, DWORD controlCode, LPVOID inBuffer, DWORD inBufferSize);
    void SetSerialPort(CString portName);
    BOOL ConfigureSerialPort(SERIAL_CONFIG serialConfig);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MFCAPPLICATION2_DIALOG };
#endif

};