#pragma once

#include "resource.h"
#include <afxwin.h> // Potrebno za MFC tipove kao što su CWnd, CDataExchange, CDialogEx, CComboBox, CString
#include <afxext.h> // Potrebno za CDialogEx

class CSettingsDialog : public CDialogEx {
public:
    CSettingsDialog(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SETTINGS_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnBnClickedSave();
    afx_msg void OnBnClickedCancel();

private:
    CComboBox m_ComboPort;
    CComboBox m_ComboBaudRate;
    CComboBox m_ComboDataBits;
    CComboBox m_ComboStopBits;
    CComboBox m_ComboParity;
    CString m_SelectedPort;
    CString m_SelectedBaudRate;
    CString m_SelectedDataBits;
    CString m_SelectedStopBits;
    CString m_SelectedParity;
};
