#pragma once


// CCommonAlgoDlg 对话框

class CCommonAlgoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCommonAlgoDlg)

public:
	CCommonAlgoDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CCommonAlgoDlg();
	BOOL OnInitDialog();
// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
};
CCommonAlgoDlg& GetCommonAlgoDlg();
