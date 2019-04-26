#include <exception>  
#include "stdafx.h"
#include "malloc.h"
#include  "wx_helper.h"
#include <tchar.h>
#include  <list>


//#region 获取群成员列表
typedef void(__stdcall *PLFUN)(void);

std::list<chat_room_user_info> parse_user_list(const DWORD addr)
{
	std::list<chat_room_user_info> list;
	if (addr == 0)
	{
		return list;
	}
	DWORD userAdd = addr;
	wchar_t test[0x100] = { 0 };
	wchar_t tempWxid[0x100] = { 0 };
	DWORD userList = *((DWORD *)addr);
	DWORD testTmp = addr + 0xB0;
	const int len = *reinterpret_cast<int *>(testTmp);
	//存放数据
	for (int i = 0; i < len; i++)
	{
		DWORD temWxidAdd = userList + (i * 20);
		swprintf_s(tempWxid, L"%s", *((LPVOID *)temWxidAdd));

		//获取用户信息
		chat_room_user_info user = get_user_info_by_wid(tempWxid);

		//list.insert(info);
		list.push_back(user); //追加到链表最后
		WCHAR titles[255] = { 0 };
		swprintf_s(titles, L"测试测试:%s,%s,%s", user.wxid, user.username, user.nickname);
		OutputDebugStringW(titles);
	}
	return list;
}

std::list<chat_room_user_info> get_chat_room_user_list(wchar_t* chat_room_wid)
{
	//40C0F0
	DWORD arrH = GetWechatWinDllAddress() + 0x40C0F0;
	DWORD sqlHWNDAdd = GetWechatWinDllAddress() + 0x2AD830;
	DWORD callAdd = GetWechatWinDllAddress() + 0x414990;
	DWORD handleAdd = GetWechatWinDllAddress() + 0x40CBD0;
	DWORD list = 0;
	char buff[0x164] = { 0 };
	char userListBuff[0x174] = { 0 };

	wx_str_5 wid = {};
	wid.str = chat_room_wid;
	wid.len = wcslen(chat_room_wid);
	wid.maxLen = wcslen(chat_room_wid) * 2;
	auto asm_wid = reinterpret_cast<char*>(&wid.str); //首位指针
	//获取群用户
	__asm {
		lea ecx, buff[16]
		call arrH
		lea eax, buff[16]
		push eax
		mov ebx, asm_wid
		push ebx
		call sqlHWNDAdd
		mov ecx, eax
		call callAdd
		lea eax, buff
		push eax
		lea ecx, buff[16]
		call handleAdd
		mov list, eax
		//sub esp, 0xC
	}
	return parse_user_list(list);
}

chat_room_user_info get_user_info_by_wid(wchar_t* wid)
{
	chat_room_user_info user = {};
	DWORD base_addr = GetWechatWinDllAddress();

	DWORD callAdd = base_addr + 0x266D30;
	DWORD callAdd2 = base_addr + 0x4BE50;
	DWORD callAdd3 = base_addr + 0x47AE60;


	wx_str_5 pWxid = { 0 };
	pWxid.str = wid;
	pWxid.len = wcslen(wid);
	pWxid.maxLen = wcslen(wid) * 2;
	char* asm_wid = (char*)&pWxid.str;

	//buffer to save info
	char buff[0xBF8] = { 0 };
	char* asmBuff = buff;

	__asm {
		mov edi, asmBuff
		push edi
		sub esp, 0x14
		mov eax, asm_wid
		mov ecx, esp
		push eax
		call callAdd3
		call callAdd2
		call callAdd
	}

	/*8 wxid
	1C 账号
	30 v1
	50 备注
	64 昵称*/
	LPVOID textWxid = *((LPVOID *)((DWORD)buff + 0x8));
	LPVOID textUser = *((LPVOID *)((DWORD)buff + 0x1C));
	LPVOID textNick = *((LPVOID *)((DWORD)buff + 0x64));
	LPVOID textV1 = *((LPVOID *)((DWORD)buff + 0x30));

	swprintf_s(user.wxid, L"%s", textWxid);
	swprintf_s(user.username, L"%s", textUser);
	swprintf_s(user.nickname, L"%s", textNick);
	swprintf_s(user.v1, L"%s", textV1);
	//test 检查是否是空,如果是空字符串就返回一个
	if (0 == wcscmp(L"(null)", user.wxid))
	{
		swprintf_s(user.wxid, L"");
		swprintf_s(user.username, L"");
		swprintf_s(user.nickname, L"");
		swprintf_s(user.v1, L"");
		user.success = false;
	}
	else
	{
		user.success = true;
	}
	//赋值
	return user;
}

//#endregion 获取群成员列表


//#region 获取好友列表
typedef std::list<DWORD> DWORD_LIST;
typedef std::list<std::wstring> WSTRING_LIST;

typedef struct _RAW_WX_NODE
{
	union
	{
		DWORD dwStart;
		PDWORD pStartNode;
	} ul;

	DWORD dwTotal;
} RAW_WX_NODE, *RAW_WXNODE;


void traversal_wx_nodes(PDWORD pWxNode, DWORD_LIST& dwListAddr, DWORD dwStart, DWORD dwTotal)
{
	if (dwListAddr.size() >= dwTotal)return;
	for (int i = 0; i < 3; i++)
	{
		DWORD dwAddr = pWxNode[i];
		DWORD_LIST::iterator iterator;
		iterator = std::find(dwListAddr.begin(), dwListAddr.end(), dwAddr);
		if (iterator == dwListAddr.end() && dwAddr != dwStart)
		{
			//递归查找
			dwListAddr.push_back(dwAddr);
			traversal_wx_nodes((PDWORD)dwAddr, dwListAddr, dwStart, dwTotal);
		}
	}
}

void parse()
{
}

typedef struct _WXTEXT
{
	PWSTR buffer;
	UINT length;
	UINT maxLen;
	UINT fill;
	UINT fill2;
} WXTEXT, *PWXTEXT;

std::list<we_chat_user> get_friends_list(int type)
{
	std::list<we_chat_user> list;
	auto pBase = (PDWORD)(GetWechatWinDllAddress() + 0x113227C);
	DWORD dwWxUserLinkedListAddr = (*pBase) + 0x24 + 0x68; //链表的地址
	RAW_WXNODE wxFriendsNode = (RAW_WXNODE)dwWxUserLinkedListAddr;
	DWORD_LIST listFriendAddr;
	if (wxFriendsNode == nullptr) {
		return list;
	}

	//if (0 != IsBadReadPtr((void*)wxFriendsNode, sizeof(DWORD))) {
	//	return list;
	//}

	traversal_wx_nodes(wxFriendsNode->ul.pStartNode, listFriendAddr, wxFriendsNode->ul.dwStart, wxFriendsNode->dwTotal);
	DWORD_LIST::iterator itor;
	for (itor = listFriendAddr.begin(); itor != listFriendAddr.end(); itor++)
	{
		we_chat_user user = { 0 };
		DWORD dwAddr = *itor;
		WXTEXT wxId = *((PWXTEXT)(dwAddr + 0x10));
		WXTEXT account = *((PWXTEXT)(dwAddr + 0x30));
		WXTEXT remark = *((PWXTEXT)(dwAddr + 0x78));
		WXTEXT nickname = *((PWXTEXT)(dwAddr + 0x8C));
		WXTEXT avatar = *((PWXTEXT)(dwAddr + 0x11C));
		WXTEXT bigAvatar = *((PWXTEXT)(dwAddr + 0x130));
		WXTEXT addType = *((PWXTEXT)(dwAddr + 0x294));

		swprintf_s(user.wxid, L"%s", wxId.buffer);
		swprintf_s(user.account, L"%s", account.buffer);
		swprintf_s(user.username, L"%s", account.buffer);
		swprintf_s(user.remark, L"%s", remark.buffer);
		swprintf_s(user.nickname, L"%s", nickname.buffer);
		swprintf_s(user.avatar, L"%s", avatar.buffer);
		//swprintf_s(user.addType, L"%s", addType.buffer);
		/*if (user.remark == "(null)") {

		}*/

		list.push_back(user);
	}
	return list;
}

//#endregion


//发送xml消息
void wx_send_xml_message(wchar_t* wxid, wchar_t* fWxid, wchar_t* title, wchar_t* content, wchar_t* pic)
{
	wx_str_5 pWxid = { 0 };
	wx_str_5 pFromWxid = { 0 };
	wx_str_3 pXML = { 0 };
	wchar_t temWxid[0x100] = { 0 };
	wchar_t fromWxid[0x100] = { 0 };
	wchar_t xml[0x2000] = { 0 };

	wx_str_5 pIcons = { 0 };
	char* asmXml = (char*)&pXML.pStr;
	char* asmWxid = (char*)&pWxid.str;
	char* asmFWxid = (char*)&pFromWxid.str;
	char* asmIcon = (char*)&pIcons.str;
	char buff1[0x24] = { 0 };
	char buff2[0x44] = { 0 };
	char maxBuff[0x77C] = { 0 };

	wchar_t iconPath[0x9C] = { 0 };
	swprintf_s(iconPath,
		L"C:\\Users\\szjy\\Documents\\WeChat Files\\HeDaDa-Alone\\Attachment\\4e590e84fc402a67994d4116bcf3bde9_t.jpg");
	pIcons.str = iconPath;
	pIcons.len = wcslen(iconPath);
	pIcons.maxLen = wcslen(iconPath) * 2;
	pIcons.f1 = 0;
	pIcons.f2 = 0;

	char buff4[0x77C] = { 0 };

	swprintf_s(temWxid, L"%s", wxid);
	pWxid.str = temWxid;
	pWxid.len = wcslen(temWxid);
	pWxid.maxLen = wcslen(temWxid) * 2;
	pWxid.f1 = 0;
	pWxid.f2 = 0;


	swprintf_s(fromWxid, L"%s", fWxid);
	pFromWxid.str = fromWxid;
	pFromWxid.len = wcslen(fromWxid);
	pFromWxid.maxLen = wcslen(fromWxid) * 2;
	pFromWxid.f1 = 0;
	pFromWxid.f2 = 0;

	swprintf_s(
		xml,
		L"<msg><fromusername>zhaojunlike</fromusername><scene>0</scene><commenturl></commenturl><appmsg appid=\"wxa4aafd8a14335fc9\" sdkver=\"0\"><title>%s</title><des>%s</des><action>view</action><type>5</type><showtype>0</showtype><content></content><url>http://www.baidu.com</url><dataurl></dataurl><lowurl></lowurl><thumburl></thumburl><messageaction></messageaction><recorditem><![CDATA[]]></recorditem><extinfo></extinfo><sourceusername></sourceusername><sourcedisplayname></sourcedisplayname><commenturl></commenturl><appattach><totallen>0</totallen><attachid></attachid><emoticonmd5></emoticonmd5><fileext></fileext><cdnthumburl>%s</cdnthumburl><aeskey>8884716e4ea321a5da7e16c25e5f6f3c</aeskey><cdnthumbaeskey>8884716e4ea321a5da7e16c25e5f6f3c</cdnthumbaeskey><encryver>0</encryver><cdnthumblength>3386</cdnthumblength><cdnthumbheight>96</cdnthumbheight><cdnthumbwidth>96</cdnthumbwidth></appattach></appmsg><appinfo><version>1</version><appname>Hezone</appname></appinfo></msg>",
		title, content, pic);

	pXML.pStr = xml;
	pXML.strLen = wcslen(xml);
	pXML.strMaxLen = wcslen(xml) * 2;


	DWORD callAdd = GetWechatWinDllAddress() + 0x22B990;
	DWORD callSendAdd = GetWechatWinDllAddress() + 0x22BBA0;

	DWORD sendP = GetWechatWinDllAddress() + 0x11393AC;
	//SetDlgItemText(Dlg, XML_LOG, xml);
	__asm {
		push 0x5
		lea eax, buff1
		push eax
		lea eax, buff2
		push eax
		mov eax, asmXml
		push eax
		mov ebx, asmWxid
		push ebx
		mov eax, asmFWxid
		push eax
		lea eax, maxBuff
		push eax
		call callAdd
		push sendP
		push sendP
		push 0x0
		lea eax, maxBuff
		push eax
		call callSendAdd
	}
}


//发送个人的名片信息
void wx_send_share_user_card(wchar_t* wxid, wchar_t* fWxid, wchar_t* name)
{
	wx_str_3 pWxid = { 0 };
	wx_str_3 pXml = { 0 };
	wchar_t xml[0x2000] = { 0 };
	pWxid.pStr = wxid;
	pWxid.strLen = wcslen(wxid);
	pWxid.strLen = wcslen(wxid) * 2;

	//具体xml包含的信息需要自己去进行一次替换
	swprintf_s(
		xml,
		L"<?xml version=\"1.0\"?><msg bigheadimgurl=\"http://wx.qlogo.cn/mmhead/ver_1/7IiaGRVxyprWcBA9v2IA1NLRa1K5YbEX5dBzmcEKw4OupNxsYuYSBt1zG91O6p07XlIOQIFhPCC3hU1icJMk3z28Ygh6IhfZrV4oYtXZXEU5A/0\" smallheadimgurl=\"http://wx.qlogo.cn/mmhead/ver_1/7IiaGRVxyprWcBA9v2IA1NLRa1K5YbEX5dBzmcEKw4OupNxsYuYSBt1zG91O6p07XlIOQIFhPCC3hU1icJMk3z28Ygh6IhfZrV4oYtXZXEU5A/132\" username=\"%s\" nickname=\"%s\" fullpy=\"?\" shortpy=\"\" alias=\"%s\" imagestatus=\"3\" scene=\"17\" province=\"湖北\" city=\"中国\" sign=\"\" sex=\"2\" certflag=\"0\" certinfo=\"\" brandIconUrl=\"\" brandHomeUrl=\"\" brandSubscriptConfigUrl= \"\" brandFlags=\"0\" regionCode=\"CN_Hubei_Wuhan\" />",
		fWxid, name, fWxid);
	pXml.pStr = xml;
	pXml.strLen = wcslen(xml);
	pXml.strMaxLen = wcslen(xml) * 2;

	char* asmWxid = (char *)&pWxid.pStr;
	char* asmXml = (char *)&pXml.pStr;
	char buff[0x20C] = { 0 };
	DWORD callAdd = GetWechatWinDllAddress() + 0x2DA0F0;
	__asm {
		mov eax, asmXml
		push 0x2A
		mov edx, asmWxid
		push 0x0
		push eax
		lea ecx, buff
		call callAdd
		add esp, 0xC
	}
}


//提权
bool ElevatePrivileges()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	tkp.PrivilegeCount = 1;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return FALSE;
	LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
	{
		return FALSE;
	}

	return TRUE;
}


void close_current_process()
{
	const HANDLE handle = GetCurrentProcess();
	if (handle != nullptr)
	{
		TerminateProcess(handle, 0);
	}
}

#pragma region 微信发送消息部分

//发送消息的库
struct WX_TEXT
{
	wchar_t* pStr; //消息 4
	int strLen; //内容的长度 8
	int iStrLen; //最后的文本就是wxid*2 8
	char* apstr;
	int iastrLen;
};


struct WeChatAtUserStr
{
	char* pStr; //消息 4
	char* strLen; //内容的长度 8
	char* iStrLen; //最后的文本就是wxid*2 8
	char* apstr;
	int iastrLen;
};

void wx_send_text_message(wchar_t* wxId, wchar_t* message, wchar_t* atId)
{
	//1.构造发送的人
	WX_TEXT recv = { 0 };
	recv.pStr = wxId;
	recv.strLen = wcslen(wxId);
	recv.iStrLen = 2 * recv.strLen;

	//2.构造消息
	WX_TEXT msg = { 0 };
	msg.pStr = message;
	msg.strLen = wcslen(message);
	msg.iStrLen = 2 * msg.strLen;

	//获取数据
	int* asmWxId = (int*)&recv.pStr; //这个直接是地址
	int* asmMsg = (int*)&msg.pStr; //消息

	//3.构造群聊At人的消息
	WX_TEXT atUser = { 0 };
	atUser.pStr = atId; //unicon
	atUser.strLen = wcslen(atId); //0
	atUser.iStrLen = 2 * wcslen(atId);

	char* eaxAddr = (char*)&atUser;
	//3.构造地址 12 位
	WeChatAtUserStr atReal = { 0 };
	//1106FFA0  10E60858  UNICODE "wxid_4czwjeqfe1ax22"

	atReal.pStr = eaxAddr;
	atReal.strLen = eaxAddr + 20;
	atReal.iStrLen = eaxAddr + 20;

	int* asmAt = (int*)&atReal.pStr; //at的人的堆栈地址

	if (wcslen(atId) == 0)
	{
		asmAt = 0x0;
	}
	char buff[0x81C] = { 0 };
	//call的地址
	DWORD sendCallAddr = GetWechatWinDllAddress() + WECHAT_SEND_MSG_CALL_RL_ADDRESS;
	//执行汇编,eax一直保持是0,发送群消息的时候
	__asm {
		mov edx, asmWxId
		mov eax, asmAt
		push 0x1
		push eax
		mov ebx, asmMsg
		push ebx
		lea ecx, buff
		call sendCallAddr
		add esp, 0xC
	}
}

void sqlite3_prepare_v2(LPVOID pDB, char* pszSql, LPVOID* pStmt)
{
	int nSqlLen = strlen(pszSql);
	//sqlite call的地址
	DWORD dwSqlite3PrepareV2Call = GetWechatWinDllAddress() + 0x7692A0;
	__asm
	{
		mov edx, pszSql
		mov ecx, pDB
		push 0x0
		mov esi, pStmt
		push esi
		push 0x0
		push 0x1
		mov eax, nSqlLen
		push eax
		call dwSqlite3PrepareV2Call
		add esp, 0x14
	}
}
#pragma endregion


#pragma region 微信消息接受部分

//hook收消息地址 安装钩子函数,

const DWORD _WCHAR_WIN_DLL_ADDR = GetWechatWinDllAddress();


//1.保存所有的堆栈地址
DWORD cEax = 0;
DWORD cEcx = 0;
DWORD cEdx = 0;
DWORD cEbx = 0;
DWORD cEsp = 0;
DWORD cEbp = 0;
DWORD cEsi = 0;
DWORD cEdi = 0;
DWORD retCallAdd = 0;
DWORD _RecvRetAdd = 0;
DWORD _RecRetCallAdd2 = 0;
//消息回调
RecvMessageCallbackFunc _RecvCallBackFun = 0;
//从内存中读取消息内容
VOID GetMemoryMessageInfo(DWORD addr)
{
	DWORD wxidAdd = addr - 0x1A0; //微信消息的来源表示
	DWORD wxid2Add = addr - 0xCC; //如果是群聊，这个就是发送人的信息
	DWORD messageAdd = addr - 0x178;

	WCHAR wxid[0x30] = { 0 };
	WCHAR senderId[0x30] = {};
	WCHAR message[0x8000] = { 0 };
	WCHAR buff[0x8000] = { 0 };
	WCHAR nickname[0x30] = L"";
	WCHAR senderName[0x30] = L"";

	//定义一个消息
	struct WECHAT_RECV_MESSAGE msg;
	//是否是群消息
	if (*(LPVOID *)wxid2Add <= 0x0)
	{
		//senderId = (WCHAR*)"";
		swprintf_s(senderId, L"%s", "");
		swprintf_s(buff, L"%s wxid:%s 消息内容:%s\r\n", buff, *((LPVOID *)wxidAdd), *((LPVOID *)messageAdd));
	}
	else
	{
		swprintf_s(senderId, L"%s", *((LPVOID *)wxid2Add));
		swprintf_s(buff, L"%s 群ID:%s 发送者ID:%s 消息内容:%s\r\n", buff, *((LPVOID *)wxidAdd), *((LPVOID *)wxid2Add),
			*((LPVOID *)messageAdd));
	}
	swprintf_s(wxid, L"%s", *((LPVOID *)wxidAdd));
	swprintf_s(message, L"%s", *((LPVOID *)messageAdd));

	//最后进行转码
	msg.wxid = UnicodeToUTF8(wxid);
	msg.senderWxId = UnicodeToUTF8(senderId);
	msg.content = UnicodeToUTF8(message);
	msg.nickname = UnicodeToUTF8(nickname);
	msg.senderName = UnicodeToUTF8(senderName);


	//通过ws发送
	if (_RecvCallBackFun != nullptr)
	{
		_RecvCallBackFun(msg);
	}
}


VOID __declspec(naked) _HookRecFunc()
{
	__asm {
		mov cEax, eax
		mov cEcx, ecx
		mov cEdx, edx
		mov cEbx, ebx
		mov cEsp, esp
		mov cEbp, ebp
		mov cEsi, esi
		mov cEdi, edi
	}
	//跳转到我们自己的函数
	GetMemoryMessageInfo(cEsi);
	retCallAdd = _WCHAR_WIN_DLL_ADDR + 0x1131AF8;
	_RecRetCallAdd2 = _WCHAR_WIN_DLL_ADDR + 0x24D340;
	_RecvRetAdd = _WCHAR_WIN_DLL_ADDR + 0x30575B;
	//恢复寄存器
	__asm {
		mov eax, cEax
		mov ecx, cEcx
		mov edx, cEdx
		mov ebx, cEbx
		mov esp, cEsp
		mov ebp, cEbp
		mov esi, cEsi
		mov edi, cEdi
		mov eax, retCallAdd
		call _RecRetCallAdd2
		jmp _RecvRetAdd
	}
}


BYTE _recvCallCode[HOOK_COMMAND_LEN] = { 0 }; //备份一下以前的，卸载的时候需要用得到

/*hook函数地址：WeChatWin.dll + 0x305758
[[esp]] + 0x40 这个地址就是wxid的地址  群消息和个人都是这个
[[esp]] + 0x68 这个地址就是消息的地址
[[esp]] + 0x114 这个地址就是消息的地址
[[esp]] + 0x128 未知遗传吗

[[esp]] + 0x44 这个值是长度
[[esp]] + 0x48 这个值是2被长度*/
BOOL WeChat_RecvMessage_Hook_Install(RecvMessageCallbackFunc recvCallFun)
{
	_RecvCallBackFun = recvCallFun;

	HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId()); //打开当前程序句柄

	//收发消息地址
	DWORD callAddress = _WCHAR_WIN_DLL_ADDR + WECHAT_RECV_MSG_CALL_RL_ADDRESS;

	//定义我们的地址函数指令
	BYTE jmpCode[HOOK_COMMAND_LEN] = { 0 };
	jmpCode[0] = 0xE9; //jmp
	*(DWORD *)&jmpCode[1] = (DWORD)&_HookRecFunc - (DWORD)callAddress - HOOK_COMMAND_LEN; //hook 字节的长度

	wchar_t debugBuff[0x100] = { 0 };

	//1.先备份老的call数据
	if (0 == ReadProcessMemory(hd, (LPCVOID)callAddress, _recvCallCode, HOOK_COMMAND_LEN, NULL))
	{
		return FALSE;
	}

	//2.写入数据
	if (0 == WriteProcessMemory(hd, (LPVOID)callAddress, jmpCode, HOOK_COMMAND_LEN, NULL))
	{
		return FALSE;
	}
	CloseHandle(hd); //关闭句柄
	return TRUE; //安装成功
}


//卸载钩子函数
BOOL WeChat_RecvMessage_Hook_UnInstall()
{
	HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId()); //打开当前程序句柄
	DWORD callAddress = _WCHAR_WIN_DLL_ADDR + WECHAT_RECV_MSG_CALL_RL_ADDRESS;
	if (0 == WriteProcessMemory(hd, (LPVOID)callAddress, _recvCallCode, HOOK_COMMAND_LEN, NULL))
	{
		return FALSE;
	}
	CloseHandle(hd); //关闭句柄
	return TRUE;
}

#pragma endregion


#pragma region 系统状态
//call 的地址
DWORD STATE_CALL = _WCHAR_WIN_DLL_ADDR + 0x54380;
//恢复地址
DWORD STATE_BACKCALL = _WCHAR_WIN_DLL_ADDR + 0x321349;
DWORD STATE_CHANGE_CALLBACK;
VOID __declspec(naked)  _WeChatStateChangeFunc()
{
	//10131344    E8 3730D3FF     call WeChatWi.0FE64380                   ; Fucker
	//<?xml version="1.0" encoding="utf-8"?>
	//<CheatTable>
	//	<CheatEntries>
	//	<CheatEntry>
	//	< ID>0 < / ID >
	//	<Description>"No description"< / Description>
	//	<LastState Value = "9460301" RealAddress = "0FE10000" / >
	//	< VariableType>4 Bytes< / VariableType>
	//	<Address>WeChatWin.dll< / Address>
	//	< / CheatEntry>
	//	< / CheatEntries>
	//	< / CheatTable>

	//call 0FE64380
	__asm {
		mov cEax, eax
		mov cEcx, ecx
		mov cEdx, edx
		mov cEbx, ebx
		mov cEsp, esp
		mov cEbp, ebp
		mov cEsi, esi
		mov cEdi, edi
	}
	DWORD state;
	//6D4380
	__asm {
		mov eax, cEax
		mov ecx, cEcx
		mov edx, cEdx
		mov ebx, cEbx
		mov esp, cEsp
		mov ebp, cEbp
		mov esi, cEsi
		mov edi, cEdi
		//mov eax, retCallAdd
		call STATE_CALL
		//再备份
		mov cEax, eax
		mov cEcx, ecx
		mov cEdx, edx
		mov cEbx, ebx
		mov cEsp, esp
		mov cEbp, ebp
		mov cEsi, esi
		mov cEdi, edi
		push edx
		call STATE_CHANGE_CALLBACK
		mov eax, cEax
		mov ecx, cEcx
		mov edx, cEdx
		mov ebx, cEbx
		mov esp, cEsp
		mov ebp, cEbp
		mov esi, cEsi
		mov edi, cEdi
		jmp STATE_BACKCALL
	}
}
bool WeChatStateChangeHookInstall(DWORD callback) {
	STATE_CHANGE_CALLBACK = callback;
	HANDLE hd = GetCurrentProcess();
	DWORD callAddress = _WCHAR_WIN_DLL_ADDR + 0x321344;
	//定义我们的地址函数指令
	BYTE jmpCode[HOOK_COMMAND_LEN] = { 0 };
	jmpCode[0] = 0xE9; //jmp
	//需要写入的hook代码
	*(DWORD *)&jmpCode[1] = (DWORD)&_WeChatStateChangeFunc - (DWORD)callAddress - HOOK_COMMAND_LEN; //hook 字节的长度
	//写入内存
	WriteProcessMemory(hd, (LPVOID)callAddress, jmpCode, HOOK_COMMAND_LEN, NULL);


	return true;
}

#pragma endregion
