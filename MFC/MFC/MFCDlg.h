
// MFCDlg.h : 头文件
//


#pragma once

//#include "customLib.h"

struct Argument
{
	char*	pcFileName;
	int		iIters;
	int		iKDTreeDepth;
	int		iInitTemp;
	int		iSampleTimes;
	int		iDesSpeed;	
	bool	bRealTime;
};

// CMFCDlg 对话框
class CMFCDlg : public CDialogEx
{
// 构造
public:
	CMFCDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFC_DIALOG };
	Argument sArgu;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton2();
};

CMFCDlg& GetMFCDlg();