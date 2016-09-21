// SimulatedAnnealDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC.h"
#include "SimulatedAnnealDlg.h"
#include "afxdialogex.h"

#include "MFCDlg.h"

// CSimulatedAnnealDlg 对话框
CSimulatedAnnealDlg& GetSimulatedAnnealAlgoDlg()
{
	static CSimulatedAnnealDlg dlg;
	return dlg;
}


IMPLEMENT_DYNAMIC(CSimulatedAnnealDlg, CDialogEx)

CSimulatedAnnealDlg::CSimulatedAnnealDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSimulatedAnnealDlg::IDD, pParent)
{

}

CSimulatedAnnealDlg::~CSimulatedAnnealDlg()
{
}

void CSimulatedAnnealDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSimulatedAnnealDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CSimulatedAnnealDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CSimulatedAnnealDlg 消息处理程序
BOOL CSimulatedAnnealDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetDlgItemInt(IDC_EDIT1, GetMFCDlg().sArgu.iIters);
	SetDlgItemInt(IDC_EDIT2, GetMFCDlg().sArgu.iKDTreeDepth);
	SetDlgItemInt(IDC_EDIT3, GetMFCDlg().sArgu.iInitTemp);
	SetDlgItemInt(IDC_EDIT4, GetMFCDlg().sArgu.iSampleTimes);
	SetDlgItemInt(IDC_EDIT5, GetMFCDlg().sArgu.iDesSpeed);

	return true;
}

void CSimulatedAnnealDlg::OnBnClickedButton1()
{
	// TODO: ÔÚ´ËÌí¼Ó¿Ø¼þÍ¨Öª´¦Àí³ÌÐò´úÂë
	if( !GetDlgItemInt(IDC_EDIT1) || !GetDlgItemInt(IDC_EDIT2) || !GetDlgItemInt(IDC_EDIT3) || !GetDlgItemInt(IDC_EDIT4) || !GetDlgItemInt(IDC_EDIT5))
	{
		MessageBox(_T("参数输入有误！"));
	}
	else if ( GetDlgItemInt(IDC_EDIT3) > 100)
	{
		MessageBox(_T("下降速度超过100！"));
	}
	else 
	{
		GetMFCDlg().sArgu.iIters =  GetDlgItemInt(IDC_EDIT1);
		GetMFCDlg().sArgu.iKDTreeDepth = GetDlgItemInt(IDC_EDIT2);
		GetMFCDlg().sArgu.iInitTemp = GetDlgItemInt(IDC_EDIT5);
		GetMFCDlg().sArgu.iSampleTimes = GetDlgItemInt(IDC_EDIT4);
		GetMFCDlg().sArgu.iDesSpeed = GetDlgItemInt(IDC_EDIT3);
		CDialogEx::OnOK();
	}
}
