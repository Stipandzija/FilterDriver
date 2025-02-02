#pragma once
#include "afxwin.h"

class CSettingsDialog : public CDialogEx
{
    DECLARE_DYNAMIC(CSettingsDialog)
    DECLARE_MESSAGE_MAP()
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SETTINGS_DIALOG };
#endif

public:
    CSettingsDialog(CWnd* pParent = nullptr);
    virtual ~CSettingsDialog();

    BOOL OnInitDialog();
    void DoDataExchange(CDataExchange* pDX);

    CComboBox ComboPort;
    CComboBox ComboBaudRate;
    CComboBox ComboDataBits;
    CComboBox ComboStopBits;
    CComboBox ComboParity;
    BOOL stayOnTop;
    BOOL quitOnEscape;
    BOOL keepHistory;
    BOOL closePort;

    CString SelectedPort;
    CString SelectedBaudRate;
    CString SelectedDataBits;
    CString SelectedStopBits;
    CString SelectedParity;

    int SavedPortIndex;
    int SavedBaudRateIndex;
    int SavedDataBitsIndex;
    int SavedStopBitsIndex;
    int SavedParityIndex;

    void OnBnClickedSettingsSave();
    void OnBnClickedSettingsCancel();
};
