#include "pch.h"
#include "SettingsDialog.h"
#include "framework.h"
#include "MFCApplication2.h"
#include "MFCApplication2Dlg.h"
#include "afxdialogex.h"
#include "winioctl.h"
#include <string>
#include <winreg.h>
#include <afxwin.h>


class CAboutDlg : public CDialogEx {
public:
    CAboutDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()



#define IDC_EDIT1 100
#define IDC_BUTTON5 101
#define SET_SERIAL_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_DATA)
#define CONFIGURE_SERIAL_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_RELEASE_DRIVER_HANDLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_WRITE_DATA)
#define IOCTL_STOP_THREAD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEVICE_RECEIVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define BUFFER_SIZE 256 

#define INACTIVITY_TIMER_ID 1

#define NO_PARITY 0
#define ODD_PARITY 1
#define EVEN_PARITY 2

deque<CString> historyList;

BEGIN_MESSAGE_MAP(CMFCApplication2Dlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CMFCApplication2Dlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CMFCApplication2Dlg::OnBnClickedButton2)
    ON_EN_CHANGE(IDC_EDIT1, &CMFCApplication2Dlg::OnEnChangeEdit1)
    ON_BN_CLICKED(IDC_BUTTON5, &CMFCApplication2Dlg::OnBnClickedButton5)
    ON_BN_CLICKED(IDC_BUTTON_SETTINGS, &CMFCApplication2Dlg::OnBnClickedSettingsButton)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CMFCApplication2Dlg::OnBnClickedButtonConnect)
    ON_WM_KEYDOWN()
    ON_WM_CLOSE()
    ON_BN_CLICKED(Recive, &CMFCApplication2Dlg::OnBnClickedRecive)
    ON_EN_CHANGE(RECEIVE_BOX, &CMFCApplication2Dlg::OnEnChangeBox)
END_MESSAGE_MAP()

BOOL CMFCApplication2Dlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    if (QuitOnEscape) {
        RegisterHotKey(GetSafeHwnd(), 1, 0, VK_ESCAPE);
    }

    EditOutput.ModifyStyle(0, ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL);

    CFont* font = new CFont;
    font->CreatePointFont(100, L"Courier New");
    SetFont(font);

    this->devicehandle = INVALID_HANDLE_VALUE;
    AfxBeginThread(ThreadFunc, this);

    return TRUE;
}

CMFCApplication2Dlg::CMFCApplication2Dlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MFCAPPLICATION2_DIALOG, pParent), isConnected(false) {
    this->devicehandle = INVALID_HANDLE_VALUE;
}

void CMFCApplication2Dlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, EditControl);
    DDX_Control(pDX, IDC_EDIT_OUTPUT, EditOutput);
}

void CMFCApplication2Dlg::OnSysCommand(UINT nID, LPARAM lParam) {
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}



void CMFCApplication2Dlg::OnBnClickedButton1() {
    if (this->devicehandle == INVALID_HANDLE_VALUE) {
        this->devicehandle = CreateFile(L"\\\\.\\devicelinkzavrsni", GENERIC_ALL, 0, 0, OPEN_EXISTING, 0, 0);
        if (this->devicehandle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            CString errorMsg;
            errorMsg.Format(_T("Failed to open device. Error %d"), error);
            OutputDebugString(errorMsg);
            OutputDebugString(errorMsg);
            return;
        }
        else {
            this->isConnected = true;
            UpdateConnectButton();
        }
    }
}

void CMFCApplication2Dlg::OnBnClickedButton2() {
    if (this->devicehandle != INVALID_HANDLE_VALUE) {
        DeviceIoControl(this->devicehandle, IOCTL_RELEASE_DRIVER_HANDLE, NULL, 0, NULL, 0, NULL, NULL);
        BOOL result = CloseHandle(this->devicehandle);
        if (!result) {
            DWORD error = GetLastError();
            CString errorMsg;
            errorMsg.Format(L"CloseHandle error: %lu", error);
            MessageBox(errorMsg, L"Error", MB_ICONERROR);
        }
        else {
            this->devicehandle = INVALID_HANDLE_VALUE;
            this->isConnected = false;
            UpdateConnectButton();
        }
    }
}

void CMFCApplication2Dlg::OnBnClickedButton5() {
    CString strText;
    GetDlgItemText(IDC_EDIT1, strText);
    CT2CA str(strText);//konvertiran ansi string
    string stringConverted(str);
    stringConverted += '\n';

    const char* message = stringConverted.c_str();
    ULONG messageLength = static_cast<ULONG>(stringConverted.length());
    char buffer[BUFFER_SIZE];
    DWORD returnLength = 0;

    if (SettingsDialog.keepHistory) {
        historyList.push_back(strText);
        if (historyList.size() > 20) {
            historyList.pop_front();
        }
        AppendTextToOutput(strText);
    }else
       EditOutput.SetWindowText(_T(""));

    if (this->devicehandle != INVALID_HANDLE_VALUE && this->devicehandle != NULL) {
        if (!DeviceIoControl(this->devicehandle, DEVICE_SEND, (LPVOID)message, messageLength, (LPVOID)buffer, BUFFER_SIZE, &returnLength, NULL)) {
            DWORD errCode = GetLastError();
            CString errorMsg;
            errorMsg.Format(L"DeviceControl error: %lu", errCode);
            MessageBox(errorMsg, L"Error", MB_ICONERROR);
        }
        else
            isFalse = true;
    }
 
    SetDlgItemText(IDC_EDIT1, _T(""));
}

void CMFCApplication2Dlg::SendIoControl(HANDLE hDevice, DWORD controlCode, LPVOID inBuffer, DWORD inBufferSize) {
    if (hDevice != INVALID_HANDLE_VALUE) {
        DWORD bytesReturned;
        BOOL isTrue = DeviceIoControl(hDevice, controlCode, inBuffer, inBufferSize, NULL, 0, &bytesReturned, NULL);
        if (!isTrue) {
            DWORD error = GetLastError();
            CString errorMsg;
            errorMsg.Format(_T("DeviceIoControl failed with error code %d"), error);
            OutputDebugString(errorMsg);
        }
    }
    
}

void CMFCApplication2Dlg::SetSerialPort(CString devicePath) {

    SendIoControl(this->devicehandle, SET_SERIAL_PORT, (LPVOID)(LPCTSTR)devicePath, (devicePath.GetLength() + 1) * sizeof(TCHAR));
}



BOOL CMFCApplication2Dlg::ConfigureSerialPort(SERIAL_CONFIG serialConfig) {

    DWORD bytesReturned;
    BOOL result = DeviceIoControl(this->devicehandle, CONFIGURE_SERIAL_PORT, &serialConfig, sizeof(serialConfig), NULL, 0, &bytesReturned, NULL);

    if (!result) {
        DWORD err = GetLastError();
        CString errorMsg;
        errorMsg.Format(L"DeviceIoControl failed with error code %d", err);
        OutputDebugString(errorMsg);
        if (this->devicehandle != INVALID_HANDLE_VALUE) {
            CloseHandle(this->devicehandle);
            this->devicehandle = INVALID_HANDLE_VALUE;
            return false;
        }

        return true;
    }
}


void CMFCApplication2Dlg::OnBnClickedSettingsButton() {
    if (SettingsDialog.DoModal() == IDOK) {
        SelectedPort = SettingsDialog.SelectedPort;
        StayOnTop = SettingsDialog.stayOnTop;
        QuitOnEscape = SettingsDialog.quitOnEscape;
        KeepHistory = SettingsDialog.keepHistory;
        ClosePortWhenInactive = SettingsDialog.closePort;

        if (SettingsDialog.stayOnTop) {
            SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        else {
            SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }

        if (SettingsDialog.quitOnEscape) {
            RegisterHotKey(GetSafeHwnd(), 1, 0, VK_ESCAPE);
        }
        else {
            UnregisterHotKey(GetSafeHwnd(), 1);
        }
        
    }
}
CString GetDevicePathFromComPort(CString comPort) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        OutputDebugString(_T("Failed to open registry key."));
        return _T("");
    }

    DWORD index = 0;
    TCHAR valueName[256];
    BYTE data[256];
    DWORD valueNameSize = 256;
    DWORD dataSize = 256;
    DWORD type;

    while (RegEnumValue(hKey, index, valueName, &valueNameSize, NULL, &type, data, &dataSize) == ERROR_SUCCESS) {
        if (type == REG_SZ) {
            CString value((TCHAR*)data);
            if (value == comPort) {
                RegCloseKey(hKey);
                CString devicePath(valueName);
                return devicePath;
            }
        }
        index++;
        valueNameSize = 256;
        dataSize = 256;
    }

    RegCloseKey(hKey);
    CString errorMsg;
    errorMsg.Format(_T("Failed to find device path for COM port: %s"), comPort);
    OutputDebugString(errorMsg);

    return _T("");
}



void CMFCApplication2Dlg::AppendTextToOutput(CString text) {
    text += _T("\r\n");

    int lenght = EditOutput.GetWindowTextLength();
    EditOutput.SetSel(lenght, lenght);

    EditOutput.ReplaceSel(text);

    EditOutput.LineScroll(EditOutput.GetLineCount());
    EditOutput.SetSel(EditOutput.GetWindowTextLength(), EditOutput.GetWindowTextLength());
}

void CMFCApplication2Dlg::OnTimer(UINT_PTR ID) {
    if (ID == INACTIVITY_TIMER_ID) {
        if (this->devicehandle != INVALID_HANDLE_VALUE) {
            CloseHandle(this->devicehandle);
            this->devicehandle = INVALID_HANDLE_VALUE;
            this->isConnected = false;
            GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Disconnected - click to connect"));
            KillTimer(INACTIVITY_TIMER_ID);
        }
    }
    CDialogEx::OnTimer(ID);
}
void CMFCApplication2Dlg::UpdateConnectButton() {
    if (this->isConnected) {
        GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Connected - click to disconnect"));
    }
    else {
        GetDlgItem(IDC_BUTTON_CONNECT)->SetWindowText(_T("Disconnected - click to connect"));
    }
}
void CMFCApplication2Dlg::OnBnClickedButtonConnect() {
    if (this->isConnected) {
        if (this->devicehandle != INVALID_HANDLE_VALUE) {
            BOOL result = DeviceIoControl(this->devicehandle, IOCTL_RELEASE_DRIVER_HANDLE, NULL, 0, NULL, 0, NULL, NULL);
            if (!result) {
                DWORD err = GetLastError();
                CString errorMsg;
                errorMsg.Format(L"OnBnClickedButtonConnect() DeviceIoControl error %d", err);
                OutputDebugString(errorMsg);
            }
            else {
                CloseHandle(this->devicehandle);
                this->devicehandle = INVALID_HANDLE_VALUE;
                this->isConnected = false;
                UpdateConnectButton();
            }
        }
    }
    else {

        CString comPort = SettingsDialog.SelectedPort;
        DevicePath = GetDevicePathFromComPort(comPort);

        if (DevicePath.IsEmpty()) {
            OutputDebugString(_T("Failed to to find device path for selected COM port: ") + DevicePath);
            return;
        }

        if (this->devicehandle != INVALID_HANDLE_VALUE) {
            DeviceIoControl(this->devicehandle, IOCTL_RELEASE_DRIVER_HANDLE, NULL, 0, NULL, 0, NULL, NULL);
            CloseHandle(this->devicehandle);
            this->devicehandle = INVALID_HANDLE_VALUE;
        }
        this->devicehandle = CreateFile(L"\\\\.\\devicelinkzavrsni", GENERIC_ALL, 0, 0, OPEN_EXISTING, 0 , 0);

        if (this->devicehandle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            CString errorMsg;
            errorMsg.Format(_T("Failed to open device. Error %d"), error);
            OutputDebugString(errorMsg);
        }
        else {

            SetSerialPort(DevicePath);
            SERIAL_CONFIG serialConfig;
            serialConfig.BaudRate = _wtoi(SettingsDialog.SelectedBaudRate);
            serialConfig.WordLength = _wtoi(SettingsDialog.SelectedDataBits);
            serialConfig.StopBits = _wtoi(SettingsDialog.SelectedStopBits);
            serialConfig.StopBits = _wtoi(SettingsDialog.SelectedStopBits);

            CString parity = SettingsDialog.SelectedParity;
            if (parity == _T("none")) {
                serialConfig.Parity = NO_PARITY;
            }
            else if (parity == _T("odd")) {
                serialConfig.Parity = ODD_PARITY;
            }
            else if (parity == _T("even")) {
                serialConfig.Parity = EVEN_PARITY;
            }
            else {
                this->isConnected = true;
                UpdateConnectButton();
            }



        }
    }
}

void CMFCApplication2Dlg::OnKeyDown(UINT keyPressed, UINT repeatCount, UINT flags) {
    if (keyPressed == VK_ESCAPE && this->QuitOnEscape) {
        PostQuitMessage(0);  
    }
    else {
        CDialogEx::OnKeyDown(keyPressed, repeatCount, flags);
    }
}


void CMFCApplication2Dlg::OnEnChangeEdit1() {
    CString strText;
    GetDlgItemText(IDC_EDIT1, strText);
    SetDlgItemText(IDC_EDIT1, strText);
    CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
    pEdit->SetSel(strText.GetLength(), strText.GetLength());

}
BOOL CMFCApplication2Dlg::PreTranslateMessage(MSG* pMsg) {
    if (pMsg->message == WM_CLOSE) {
        OnClose();
        return TRUE; 
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

void CMFCApplication2Dlg::OnClose() {
    if (this->devicehandle != INVALID_HANDLE_VALUE) {
        OutputDebugString(L"OnClose function called\n");
        DWORD bytesReturned;
        BOOL result = DeviceIoControl(this->devicehandle, IOCTL_STOP_THREAD, NULL, 0, NULL, 0, &bytesReturned, NULL);
        if (!result) {
            DWORD error = GetLastError();
            CString errorMsg;
            errorMsg.Format(L"DeviceIoControl failed with error code %lu", error);
            OutputDebugString(errorMsg);
        }
        else {
            result = CloseHandle(this->devicehandle);// dodano ubacili smo pod else result 
            if (!result) {
                DWORD error = GetLastError();
                CString errorMsg;
                errorMsg.Format(L"CloseHandle error: %lu", error);
                OutputDebugString(errorMsg);
            }
            else {
                this->devicehandle = INVALID_HANDLE_VALUE;
            }
        }
    }

    CDialogEx::OnClose();
}


void CMFCApplication2Dlg::OnBnClickedRecive()
{
    char szBuffer[1024] = {};  
    DWORD bytesRead = 0;        

    if (this->devicehandle == INVALID_HANDLE_VALUE) {
        OutputDebugString(_T("Serial port not open.\n"));
        return;
    }

    if (this->devicehandle != INVALID_HANDLE_VALUE && this->devicehandle != NULL) {
        if (!DeviceIoControl(this->devicehandle, DEVICE_RECEIVE, NULL, NULL, NULL, NULL, NULL, NULL)) {
            DWORD errCode = GetLastError();
            CString errorMsg;
            errorMsg.Format(L"DeviceControl error: %lu", errCode);
            OutputDebugString(errorMsg);
        }
    }

}


void CMFCApplication2Dlg::OnEnChangeBox()
{
    CString currentText;
    GetDlgItemText(RECEIVE_BOX, currentText);
}

UINT CMFCApplication2Dlg::ThreadFunc(LPVOID x)
{
    CMFCApplication2Dlg* app = (CMFCApplication2Dlg*)x;
    char szBuffer[50] = { 0 };
    DWORD bytesRead = 0;

    while (true) 
    {
        if (app->devicehandle != INVALID_HANDLE_VALUE && app->isFalse!=false)
        {
            BOOL result = DeviceIoControl(app->devicehandle, DEVICE_RECEIVE, NULL, NULL, szBuffer, sizeof(szBuffer), &bytesRead, NULL);
            if (result && bytesRead > 0)
            {

                CString receivedData;
                for (DWORD i = 0; i < bytesRead; i++)
                {
                    char ch = szBuffer[i];
                    if (ch >= 32 && ch <= 126)
                    {
                        receivedData.AppendChar(ch);
                    }
                    else
                        receivedData.AppendChar('.');
                }
                CTime currentTime = CTime::GetCurrentTime();
                CString dateTime = currentTime.Format(_T("%d/%m/%Y %H:%M:%S"));

                CString formattedText;
                formattedText.Format(_T("%d, %s, %s\r\n"), app->currentIndex, dateTime, receivedData);

                CString currentText;
                app->GetDlgItemText(RECEIVE_BOX, currentText);
                currentText += formattedText;
                app->currentIndex++;
                app->SetDlgItemText(RECEIVE_BOX, currentText);
            }
            app->isFalse = false;


        }
        Sleep(2000);  
    }

    return 0;
}
