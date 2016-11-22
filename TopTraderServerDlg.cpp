
// TopTraderServerDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "messageinfo.pb.h"
#include "TopTraderServer.h"
#include "TopTraderServerDlg.h"
#include "afxdialogex.h"
#include "WaitFor.h"
#include "SysHelper.h"
#include "MeMManger.h"//用于用户链的管理
using namespace std;

#ifdef _DEBUG
#	pragma comment(lib, "libprotobuf_d.lib")
#else
#	pragma comment(lib, "libprotobuf.lib")
#endif

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "../Lib/HPSocket/x86/HPSocket_UD.lib")
	#else
		#pragma comment(lib, "../Lib/HPSocket/x86/HPSocket_U.lib")
	#endif
#else
	#ifdef _DEBUG
		#pragma comment(lib, "../Lib/HPSocket/x86/HPSocket_UD.lib")
	#else
		#pragma comment(lib, "../Lib/HPSocket/x86/HPSocket_U.lib")
	#endif
#endif

// CServerDlg dialog

#define DEFAULT_ADDRESS	_T("0.0.0.0")
#define DEFAULT_PORT	_T("5555")

extern MeMManger UserListManger;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTopTraderServerDlg 对话框




CTopTraderServerDlg::CTopTraderServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTopTraderServerDlg::IDD, pParent), m_Server(this)
	, m_lClientCount(0L)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTopTraderServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INFO, m_Info);
	DDX_Control(pDX, IDC_START, m_Start);
	DDX_Control(pDX, IDC_STOP, m_Stop);
	DDX_Control(pDX, IDC_PORT, m_Port);
	DDX_Control(pDX, IDC_SEND_POLICY, m_SendPolicy);
	DDX_Control(pDX, IDC_THREAD_COUNT, m_ThreadCount);
	DDX_Control(pDX, IDC_MAX_CONN_COUNT, m_MaxConnCount);
	DDX_Control(pDX, IDC_ONLINE_NUM, m_OnlineNum);
}

BEGIN_MESSAGE_MAP(CTopTraderServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CTopTraderServerDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CTopTraderServerDlg::OnBnClickedStop)
	ON_MESSAGE(USER_INFO_MSG, OnUserInfoMsg)
	ON_WM_VKEYTOITEM()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_WM_COPYDATA()
	ON_MESSAGE(WM_DEALRECEIVE, &CTopTraderServerDlg::OnDealreceive)
END_MESSAGE_MAP()


// CTopTraderServerDlg 消息处理程序

BOOL CTopTraderServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	CString strTitle;
	CString strOriginTitle;
	m_SendPolicy.SetCurSel(0);
	m_ThreadCount.SetCurSel(0);
	m_MaxConnCount.SetCurSel(0);
	m_Port.SetWindowText(DEFAULT_PORT);

	::SetMainWnd(this);
	::SetInfoList(&m_Info);
	SetAppState(ST_STOPPED);

	//这里先使用已经定义的一个procobuf模板，初始化一个发送测试。
	retrieveData.set_logonresult(LOGON_RESULT_SUCCESS);//设值登录返回值

	UserInfo* userInfo = retrieveData.mutable_userinfo();//建立用户信息表，并加以初始化。	
	userInfo->set_email("yiyuan@qq.com");
	userInfo->set_mobile("123456789");
	userInfo->set_name("杰出交易员");
	userInfo->set_password("fanyong");
	userInfo->set_wxopenid("dj32dffmka93jfdfee");
	userInfo->set_status(OFFLINE);
	userInfo->set_follow(1);

	SetTimer(TIMERID,10000,0); //启动10秒钟定时中进行序列化及发送消息
	mPort = 5555;

//创建一个发送线程  //FxManger中有更多的线程实例
	dwSendthreadid = 0;
	sender = new SENDTRST;
	hSendthread = CreateThread(NULL,0,CTopTraderServerDlg::SendThreadFunc,
		                           (LPVOID)sender,0,&dwSendthreadid);
	if (hSendthread == NULL)
	{
		::LogMsg(_T("群发线程无法建立"));
		return TRUE;
	}
	CloseHandle(hSendthread);//只是关闭了一个线程句柄对象，表示我不再使用该句柄，即不对这个句柄对应的线程做任何干预了。 线程挂起


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTopTraderServerDlg::OnClose()
{
	int m = m_Server->GetState();
	if(m == ST_STARTED)
	{
		this->MessageBox(_T("退出软件请先停止运行服务器！"), _T("提示"));
		return;
	}

	::SetMainWnd(nullptr);
	__super::OnClose();
}

void CTopTraderServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTopTraderServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTopTraderServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CTopTraderServerDlg::PreTranslateMessage(MSG* pMsg)
{
	if (
			pMsg->message == WM_KEYDOWN		
			&&(	pMsg->wParam == VK_ESCAPE	 
			||	pMsg->wParam == VK_CANCEL	
			||	pMsg->wParam == VK_RETURN	
		))
		return TRUE;

	return CDialog::PreTranslateMessage(pMsg);
}

void CTopTraderServerDlg::SetAppState(EnAppState state)
{
	if(m_enState == state)
		return;

	m_enState = state;

	if(this->GetSafeHwnd() == nullptr)
		return;

	m_Start.EnableWindow(m_enState == ST_STOPPED);
	m_Stop.EnableWindow(m_enState == ST_STARTED);
	m_Port.EnableWindow(m_enState == ST_STOPPED);
	m_SendPolicy.EnableWindow(m_enState == ST_STOPPED);
	m_ThreadCount.EnableWindow(m_enState == ST_STOPPED);
	m_MaxConnCount.EnableWindow(m_enState == ST_STOPPED);
}

void CTopTraderServerDlg::OnBnClickedStart()
{
	CString strPort;
	m_Port.GetWindowText(strPort);
	USHORT usPort = (USHORT)_ttoi(strPort);

	if(usPort == 0)
	{
		MessageBox(_T("Listen Port invalid, pls check!"), _T("Params Error"), MB_OK);
		m_Port.SetFocus();
		return;
	}

	EnSendPolicy enSendPolicy = (EnSendPolicy)m_SendPolicy.GetCurSel();

	CString strThreadCount;
	m_ThreadCount.GetWindowText(strThreadCount);
	int iThreadCount = _ttoi(strThreadCount);

	if(iThreadCount == 0)
		iThreadCount = min((::SysGetNumberOfProcessors() * 2 + 2), 500);//默认为服务器核心X2+2线程，一核4线程，2核6线程
	else if(iThreadCount < 0 || iThreadCount > 500)
	{
		m_ThreadCount.SetFocus();
		return;
	}

	CString strMaxConnCount;
	m_MaxConnCount.GetWindowText(strMaxConnCount);
	int iMaxConnCount = _ttoi(strMaxConnCount);

	if(iMaxConnCount == 0)//默认最大连接数为1万在线
		iMaxConnCount = 10;
	
	iMaxConnCount *= 1000;

	SetAppState(ST_STARTING);

	Reset();
	//通过这个智能指针来选择这个服务器是什么类型的，这里声明的是PUSH
	//通信角色（Client / Server）、通信协议（TCP / UDP/HTTP）和接收模型（PUSH / PULL / PACK）
	m_Server->SetSendPolicy(enSendPolicy);
	m_Server->SetWorkerThreadCount(iThreadCount);
	m_Server->SetMaxConnectionCount(iMaxConnCount);

	//m_Server->SetFreeSocketObjPool(500);
	//m_Server->SetFreeSocketObjHold(1500);
	//m_Server->SetFreeBufferObjPool(2000);
	//m_Server->SetFreeBufferObjHold(6000);
	//m_Server->SetSocketListenQueue(2000);
	//m_Server->SetAcceptSocketCount(2000);


	if(m_Server->Start(DEFAULT_ADDRESS, usPort))
	{
		::LogServerStart(DEFAULT_ADDRESS, usPort);
		sender->m_Server = m_Server.Get();//保存好句柄，在群发线程中使用
		SetAppState(ST_STARTED);
	}
	else
	{
		::LogServerStartFail(m_Server->GetLastError(), m_Server->GetLastErrorDesc());
		SetAppState(ST_STOPPED);
	}
}

void CTopTraderServerDlg::OnBnClickedStop()
{
	SetAppState(ST_STOPPING);

	if(m_Server->Stop())
	{
		::LogServerStop();
		SetAppState(ST_STOPPED);
	}
	else
	{
		ASSERT(FALSE);
	}
}
//以下为快捷键处理listbox
int CTopTraderServerDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
	if(nKey == 'C')
		pListBox->ResetContent();
	else if(nKey == 'R')
	{
		Reset();

		CString strMsg;
		strMsg.Format(	_T(" *** Reset Statics: CC -  %u, TS - %lld, TR - %lld"),
						m_lClientCount, m_llTotalSent, m_llTotalReceived);

		::LogMsg(strMsg);
	}

	return __super::OnVKeyToItem(nKey, pListBox, nIndex);
}

//群发线程
DWORD WINAPI CTopTraderServerDlg::SendThreadFunc(LPVOID lpParameter)
{
	MSG msg;
	int length;
	CString message;
	USERLIST* m_sender;
	struct SENDTRST* sender;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);	
	int From_ClientType;//服务器收到来自客户端的操作类型
	string From_ClientTitle,From_ClientLink;
	bool show = false;
	CBufferPtr m_sendBuffer;//管理员使用的缓冲
	CommandInfo* commandInfo;//管理员使用的数据序列化
	while(true)
		{
		if(GetMessage(&msg, NULL, 0, 0))
		  {
			 switch(msg.message)
			 {
			   case WM_SENDTOALL:  //使用 PostThreadMessage(nDealThreadID,WM_SENDTOALL,0,0); 
				   //先进行解析
					sender = (struct SENDTRST *)lpParameter;
					From_ClientTitle = sender->m_retrieveData.commandinfo(0).content();
					From_ClientType = sender->m_retrieveData.commandinfo(0).type();	
					From_ClientLink = sender->m_retrieveData.commandinfo(0).link();	


				   	//下面为群发数据——————————
					if(From_ClientType ==0){  //=0为命令

						sender->m_retrieveData.clear_commandinfo();
						commandInfo = sender->m_retrieveData.add_commandinfo();
						commandInfo->set_type(1);//不能发给自己,在其它的线程中也不能发给别人,转为直播群发
						commandInfo->set_content(From_ClientTitle);
						commandInfo->set_link(From_ClientLink);
						length = sender->m_retrieveData.ByteSize();

						m_sendBuffer.Malloc(length,true);
						sender->m_retrieveData.SerializeToArray(m_sendBuffer,length);
						sender->m_retrieveData.clear_commandinfo(); 
						
						show = true;
						UserListManger.m_logLock.Lock();//需要保护一下,群发时链表不能动
						m_sender = UserListManger.m_UserList;	   
						while(m_sender){
							if(m_sender->dwConnID != sender->self) //不再发送到自己了，因为管理员发送时已经在接受回调中原路返回一次了
							    sender->m_Server->Send(m_sender->dwConnID,m_sendBuffer,length,0);
							m_sender = m_sender->next;
									 }
						UserListManger.m_logLock.Unlock();

						m_sendBuffer.Free();
					    }
					break;
			   case WM_ZHIBO://直播使用的缓冲与管理员的缓冲是不同的地方
				   	//下面为群发数据,这里过来的是已经序列化好的内容，放在sender->之中，带长度——————————
					sender = (struct SENDTRST *)lpParameter;
					UserListManger.m_logLock.Lock();//需要保护一下,群发时链表不能动
					m_sender = UserListManger.m_UserList;	   
					while(m_sender){				
							sender->m_Server->Send(m_sender->dwConnID,sender->m_sendBuffer,sender->length,0);
							m_sender = m_sender->next;
								 }
					UserListManger.m_logLock.Unlock();

					sender->m_sendBuffer.Free();
					break;
			 }

				if(show){
					CString message(From_ClientTitle.c_str());//string转CString
					::LogMsg(message);
					show = false;
				   }
			  }
		}

	return 0;
}



//处理用户消息，这里的wp,就是在使用PostInfoMsg传递过来的
LRESULT CTopTraderServerDlg::OnUserInfoMsg(WPARAM wp, LPARAM lp)
{
	info_msg* msg = (info_msg*)wp;

	::LogInfoMsg(msg);

	return 0;
}

EnHandleResult CTopTraderServerDlg::OnPrepareListen(ITcpServer* pSender, SOCKET soListen)
{
	TCHAR szAddress[40];
	int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
	USHORT usPort;

	pSender->GetListenAddress(szAddress, iAddressLen, usPort);
	::PostOnPrepareListen(szAddress, usPort);
	return HR_OK;
}

EnHandleResult CTopTraderServerDlg::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
#ifdef _DEBUG2
	::PostOnSend(dwConnID, pData, iLength);
#endif

#if (_WIN32_WINNT <= _WIN32_WINNT_WS03)
	::InterlockedExchangeAdd((volatile LONG*)&m_llTotalSent, iLength);
#else
	::InterlockedExchangeAdd64(&m_llTotalSent, iLength);
#endif
//这里是向客户端发送的指令后的响应——————————————————————————
	//::PostOnSend(dwConnID, pData, iLength);
	return HR_OK;
}



EnHandleResult CTopTraderServerDlg::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
//这个地方的代码是接收原来压力测试客户端的3秒钟后发来的数据，客户端发多少过来，就回多少过去。

#ifdef _DEBUG2
	::PostOnReceive(dwConnID, pData, iLength);
#endif

#if (_WIN32_WINNT <= _WIN32_WINNT_WS03)
/*InterlockedCompareExchange,InterlockedDecrement,InterlockedExchange,InterlockedIncrement
机制提供了一个简单的同步访问一个变量共享多个线程。 
线程可以使用不同的过程机制如果变量在共享内存。
这个函数执行InterlockedExchangeAdd原子增加价值的价值指向加数。
结果被存放在指定的地址加数。初始值的变量指向由加数返回的功能价值*/


//这里的m_llTotalReceived表示累计收到的字节数，而这里的iLength为缓冲区大小。
//每取一次iLength，m_llTotalReceived=m_llTotalReceived+iLength，取完为止。
//如果知道了希望取多少，则就可以知道是否已经完成。

//volatile的作用是： 作为指令关键字，确保本条指令不会因编译器的优化而省略，且要求每次直接读值.
	::InterlockedExchangeAdd((volatile LONG*)&m_llTotalReceived, iLength);
#else
	::InterlockedExchangeAdd64(&m_llTotalReceived, iLength);
#endif

//在这里处理客户端发来的指令及信息：——————————————————————————
//这里必须取走数据，立马返回成功，然后在自己的线程里处理群发等功能————————————————————
	sender->m_retrieveData.ParseFromArray(pData,iLength);//获取数据
	sender->self = dwConnID;
	::PostThreadMessage(dwSendthreadid,WM_SENDTOALL,0,0); //命令群发
	

	if(pSender->Send(dwConnID, pData, iLength))//立即返回
		return HR_OK;
	else
		return HR_ERROR;
}

//这里是断开连接后的处理
EnHandleResult CTopTraderServerDlg::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	iErrorCode == SE_OK ? ::PostOnClose(dwConnID)	:
	::PostOnError(dwConnID, enOperation, iErrorCode);

	Statistics();
//	在函数内做临界区
	UserListManger.DelUserFromList(dwConnID);//从链表中移走客户端
	return HR_OK;
}

//这里是客户端成功连接服务器的处理
EnHandleResult CTopTraderServerDlg::OnAccept(ITcpServer* pSender, CONNID dwConnID, SOCKET soClient)
{
	if(m_lClientCount == 0)
	{
		CCriSecLock lock(m_cs);

		if(m_lClientCount == 0)
		{
			Reset(FALSE);
		}
	}
//原子增加，表示在m_lClientCount++的操作时，受保护
	::InterlockedIncrement(&m_lClientCount);
	CString OnlineNum;
	OnlineNum.Format(_T("%d"),m_lClientCount);
	m_OnlineNum.SetWindowText(OnlineNum);
	::PostOnAccept2(dwConnID);


	USERINFO *newobj = (USERINFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(USERINFO));

	//TCHAR szAddress[40];
	//int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
	//USHORT usPort=5555;
	//pSender->GetRemoteAddress(dwConnID,szAddress,iAddressLen,usPort);
	//::LogMsg(szAddress);
	//用户信息，可以去数据库查询——————
	newobj->follow = 1;
	newobj->email = "yiyuan@qq.com";
	newobj->mobile ="13957278374";
	newobj->name = "yiyuan";
	newobj->password = "fanyong";
	newobj->status = 1;
	newobj->WxOpenId = "sdfaei3232djk3krjk";

//其实系统自在附加数据，并且这个是做保护的，把 Connection ID 和应用层数据进行绑定。这样说提升性能，我是自己解决链表与数据的。
	//USERINFO *newobj = (USERINFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(USERINFO));
	//pSender->SetConnectionExtra(dwConnID,newobj);//同时可以在接收这里取出来，系统close时会自动释放newobj

	//	在函数内做临界区
	UserListManger.AddUserToList(dwConnID,newobj);//从链表中增加客户端(已经连接)

	//查询完成可以结果消息设置porotbuf回发给客户端——————

	return HR_OK;
}

EnHandleResult CTopTraderServerDlg::OnShutdown(ITcpServer* pSender)
{
	::PostOnShutdown();
	return HR_OK;
}

//这里是用在断开连接后的状态处理：
void CTopTraderServerDlg::Statistics()
{
	if(m_lClientCount > 0)
	{
		CCriSecLock lock(m_cs);

		if(m_lClientCount > 0)
		{
			::InterlockedDecrement(&m_lClientCount);//原子减，表示在m_lClientCount++的操作时，受保护
			CString OnlineNum;
			OnlineNum.Format(_T("%d"),m_lClientCount);
			m_OnlineNum.SetWindowText(OnlineNum);
			if(m_lClientCount == 0)
			{
				::WaitWithMessageLoop(600L);
				::PostServerStatics((LONGLONG)m_llTotalSent, (LONGLONG)m_llTotalReceived);
			}
		}
	}
}

void CTopTraderServerDlg::Reset(BOOL bResetClientCount)
{
	if(bResetClientCount)//直接写个0，一般默认是int类型，数字后缀L表示long型
		m_lClientCount	= 0L;

	m_llTotalSent		= 0L;
	m_llTotalReceived	= 0L;
}


void CTopTraderServerDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此处理发送消息的程序

  static int nShowNum = 1;
/*
   if( nIDEvent == 8)

   {
	   USERLIST* m_sender;
	   
	   UserListManger.m_logLock.Lock();//需要保护一下
	   m_sender = UserListManger.m_UserList;	   
	   while(m_sender){	  	   
		   m_Server->Send(m_sender->dwConnID,m_sendBuffer,length,0);
		   m_sender = m_sender->next;
	   }
		
	   UserListManger.m_logLock.Unlock();
	   m_sendBuffer.Free();
}*/

}

BOOL CTopTraderServerDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
    string msg,Title,Time,Link,CommandText; 
	CString msg1,msg2,show;
    char *sContent = NULL;
	char *p=NULL,*pNext=NULL;
    sContent = (char*)pCopyDataStruct->lpData;  

	PySendList = NULL;
	int length,pos,count = 0;

	msg = sContent;
	msg1 = sContent;
	//14:45 注意观察|TopZb14:45|TopZb|TobZbwu2198*TopZb14:40 股指下试|TopZb14:40|TopZb|TobZbwu2198*TopZb14:36 深圳成交3330亿|TopZb14:36|TopZb|TobZbwu2198*TopZb
	//*TopZb14:40 股指下试|TopZb14:40|TopZb|TobZbwu2198*TopZb14:36 深圳成交3330亿|TopZb14:36|TopZb|TobZbwu2198*TopZb

//	p = strtok_s(sContent,"*TopZb",&pNext);//去掉最后的*|
	//14:45 注意观察|
	//opZb14:45|TopZb|TobZbwu2198*TopZb14:40 股指下试|TopZb14:40|TopZb|TobZbwu2198*TopZb14:36 深圳成交3330亿|TopZb14:36|TopZb|TobZbwu2198*TopZb

//生成一张待发送数据链接表
	pos = msg1.Find(_T("*TopZb"));
	while(pos != -1){
		SendList *m_SendList = new SendList;
		m_SendList->next = PySendList;
		m_SendList->msg = msg1.Left(pos);
		msg1.Delete(0,pos + 6);
		pos = msg1.Find(_T("*TopZb"));
		PySendList = m_SendList;
		count ++;
	}

	count = 0;//

	SendList *Tempforfree;
	while(PySendList){
		msg1 = PySendList->msg;
		pos = msg1.Find(_T("|TopZb"));
		msg2 = msg1.Left(pos);

		show.Format(_T("%s"),msg2); //放入标题

		CStringA stra(msg2.GetBuffer(0));
		msg2.ReleaseBuffer();
		Title = stra.GetBuffer(0);
		stra.ReleaseBuffer();

		if(pos == -1) pos  =0 ;
		msg1.Delete(0,pos + 6);
		pos = msg1.Find(_T("|TopZb"));
		msg2 = msg1.Left(pos);

		CStringA strb(msg2.GetBuffer(0));
		msg2.ReleaseBuffer();
		Time = strb.GetBuffer(0);
		strb.ReleaseBuffer();

		if(pos == -1) pos  =0 ;
		msg1.Delete(0,pos + 6);
		pos = msg1.Find(_T("|TopZb"));
		msg2 = msg1.Left(pos);


		CStringA strc(msg2.GetBuffer(0));
		msg2.ReleaseBuffer();
		Link = strc.GetBuffer(0);
		strc.ReleaseBuffer();

		if(pos == -1) pos  =0 ;
		msg1.Delete(0,pos + 6);

		msg2 = show;
		show.Format(_T("%s: %s"),msg1,msg2); //放入大V
		::LogMsg(show);

		CStringA strd(msg1.GetBuffer(0));
		msg1.ReleaseBuffer();
		CommandText = strd.GetBuffer(0);
		strd.ReleaseBuffer();

		//以下是需要循环添加到protobuf的项目
		commandInfo = retrieveData.add_commandinfo();//添加一个消息命令结构体CommandInfo	

		if(strcmp(CommandText.c_str(),"newday")==0){
			commandInfo->set_type(0);
			commandInfo->set_content("newday");
		}else{
		commandInfo->set_type(1);//这里的值最好也用枚举值，这样容易记，（0=操作命令  1=直播消息  2=文章链接 3=金融快讯  4=仓位信息）。
		commandInfo->set_content(Title);
			}
		commandInfo->set_link(Link);	
		count++;
		retrieveData.set_totaldatanum(count);//设定有多少个消息命令结构体CommandInfo
		Tempforfree = PySendList;
		PySendList = PySendList->next;
		delete Tempforfree;//马上释放
		     }

	length = retrieveData.ByteSize();

	UserListManger.m_SendToall_Lock.Lock();//群发时需要保护一下缓冲区
	sender->m_sendBuffer.Malloc(length,true);
	sender->length = length;
	retrieveData.SerializeToArray(sender->m_sendBuffer,length);
	::PostThreadMessage(dwSendthreadid,WM_ZHIBO,0,0); //群发
	UserListManger.m_SendToall_Lock.Unlock();//群发时需要保护一下缓冲区

	retrieveData.clear_commandinfo();//必须要清理，否则链表会越来越长

	return __super::OnCopyData(pWnd, pCopyDataStruct);
}


afx_msg LRESULT CTopTraderServerDlg::OnDealreceive(WPARAM wParam, LPARAM lParam)
{
	return 0;
}
