#pragma once


// CSimulatedAnnealDlg 对话框

class CSimulatedAnnealDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSimulatedAnnealDlg)

public:
	CSimulatedAnnealDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSimulatedAnnealDlg();
	BOOL OnInitDialog();
// 对话框数据
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
};

CSimulatedAnnealDlg& GetSimulatedAnnealAlgoDlg();
