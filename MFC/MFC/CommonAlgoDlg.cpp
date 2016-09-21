// CommonAlgoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC.h"
#include "CommonAlgoDlg.h"
#include "afxdialogex.h"

#include "MFCDlg.h"

// CCommonAlgoDlg 对话框
CCommonAlgoDlg& GetCommonAlgoDlg()
{
	static CCommonAlgoDlg dlg;
	return dlg;
}
IMPLEMENT_DYNAMIC(CCommonAlgoDlg, CDialogEx)

CCommonAlgoDlg::CCommonAlgoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCommonAlgoDlg::IDD, pParent)
{

}

CCommonAlgoDlg::~CCommonAlgoDlg()
{
}

void CCommonAlgoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCommonAlgoDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CCommonAlgoDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CCommonAlgoDlg 消息处理程序

BOOL CCommonAlgoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetDlgItemInt(IDC_EDIT1, GetMFCDlg().sArgu.iIters);
	SetDlgItemInt(IDC_EDIT2, GetMFCDlg().sArgu.iKDTreeDepth);

	return TRUE;
}


void CCommonAlgoDlg::OnBnClickedButton1()
{
	// TODO: ÔÚ´ËÌí¼Ó¿Ø¼þÍ¨Öª´¦Àí³ÌÐò´úÂë
	if( !GetDlgItemInt(IDC_EDIT1) && !GetDlgItemInt(IDC_EDIT2) && !GetDlgItemInt(IDC_EDIT3) && !GetDlgItemInt(IDC_EDIT4) && !GetDlgItemInt(IDC_EDIT5))
	{
		MessageBox(_T("参数输入有误！"));
	}
	else
	{
		GetMFCDlg().sArgu.iIters =  GetDlgItemInt(IDC_EDIT1);
		GetMFCDlg().sArgu.iKDTreeDepth = GetDlgItemInt(IDC_EDIT2);
		
		CDialogEx::OnOK();
	}
	
}
