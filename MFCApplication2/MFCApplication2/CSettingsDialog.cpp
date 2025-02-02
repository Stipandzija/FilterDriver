#include "pch.h"
#include "MFCApplication2.h"
#include "framework.h"
#include "SettingsDialog.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CSettingsDialog, CDialogEx)

CSettingsDialog::CSettingsDialog(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_SETTINGS_DIALOG, pParent)
{
}

CSettingsDialog::~CSettingsDialog()
{
}

void CSettingsDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_PORT, ComboPort);
    DDX_Control(pDX, IDC_COMBO_BAUDRATE, ComboBaudRate);
    DDX_Control(pDX, IDC_COMBO_DATABITS, ComboDataBits);
    DDX_Control(pDX, IDC_COMBO_STOPBITS, ComboStopBits);
    DDX_Control(pDX, IDC_COMBO_PARITY, ComboParity);

    DDX_Check(pDX, IDC_CHECK_STAY_ON_TOP, stayOnTop);
    DDX_Check(pDX, IDC_CHECK_QUIT_ON_ESCAPE, quitOnEscape);
    DDX_Check(pDX, IDC_CHECK_KEEP_HISTORY, keepHistory);
    DDX_Check(pDX, IDC_CHECK_CLOSE_PORT_WHEN_INACTIVE, closePort);
}

BEGIN_MESSAGE_MAP(CSettingsDialog, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_SETTINGS_SAVE, &CSettingsDialog::OnBnClickedSettingsSave)
    ON_BN_CLICKED(IDC_BUTTON_SETTINGS_CANCEL, &CSettingsDialog::OnBnClickedSettingsCancel)
END_MESSAGE_MAP()

BOOL CSettingsDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ComboPort.AddString(_T("COM1"));
    ComboPort.AddString(_T("COM2"));
    ComboPort.AddString(_T("COM3"));
    ComboPort.AddString(_T("COM4"));
    ComboPort.AddString(_T("COM5"));
    ComboPort.AddString(_T("COM6"));
    ComboPort.AddString(_T("COM7"));
    ComboPort.AddString(_T("COM8"));

    ComboBaudRate.AddString(_T("9600"));
    ComboBaudRate.AddString(_T("19200"));
    ComboBaudRate.AddString(_T("38400"));
    ComboBaudRate.AddString(_T("57600"));
    ComboBaudRate.AddString(_T("115200"));

    ComboDataBits.AddString(_T("5"));
    ComboDataBits.AddString(_T("6"));
    ComboDataBits.AddString(_T("7"));
    ComboDataBits.AddString(_T("8"));

    ComboStopBits.AddString(_T("1"));
    ComboStopBits.AddString(_T("1.5"));
    ComboStopBits.AddString(_T("2"));

    ComboParity.AddString(_T("none"));
    ComboParity.AddString(_T("odd"));
    ComboParity.AddString(_T("even"));

    if (SavedPortIndex >= 0) ComboPort.SetCurSel(SavedPortIndex);
    if (SavedBaudRateIndex >= 0) ComboBaudRate.SetCurSel(SavedBaudRateIndex);
    if (SavedDataBitsIndex >= 0) ComboDataBits.SetCurSel(SavedDataBitsIndex);
    if (SavedStopBitsIndex >= 0) ComboStopBits.SetCurSel(SavedStopBitsIndex);
    if (SavedParityIndex >= 0) ComboParity.SetCurSel(SavedParityIndex);

    return TRUE;
}

void CSettingsDialog::OnBnClickedSettingsSave()
{
    ComboPort.GetLBText(ComboPort.GetCurSel(), SelectedPort);
    ComboBaudRate.GetLBText(ComboBaudRate.GetCurSel(), SelectedBaudRate);
    ComboDataBits.GetLBText(ComboDataBits.GetCurSel(), SelectedDataBits);
    ComboStopBits.GetLBText(ComboStopBits.GetCurSel(), SelectedStopBits);
    ComboParity.GetLBText(ComboParity.GetCurSel(), SelectedParity);

    SavedPortIndex = ComboPort.GetCurSel();
    SavedBaudRateIndex = ComboBaudRate.GetCurSel();
    SavedDataBitsIndex = ComboDataBits.GetCurSel();
    SavedStopBitsIndex = ComboStopBits.GetCurSel();
    SavedParityIndex = ComboParity.GetCurSel();

    stayOnTop = IsDlgButtonChecked(IDC_CHECK_STAY_ON_TOP);
    quitOnEscape = IsDlgButtonChecked(IDC_CHECK_QUIT_ON_ESCAPE);
    keepHistory = IsDlgButtonChecked(IDC_CHECK_KEEP_HISTORY);
    closePort = IsDlgButtonChecked(IDC_CHECK_CLOSE_PORT_WHEN_INACTIVE);

    EndDialog(IDOK);
}

void CSettingsDialog::OnBnClickedSettingsCancel()
{
    EndDialog(IDCANCEL);
}