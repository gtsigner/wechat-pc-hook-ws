#pragma once
#include "stdafx.h"
#include "cJSON.h"
#include  <list>

#define WECHAT_WIN_BASE (DWORD)GetModuleHandle(_T("WeChatWin.dll"))
//二维码的偏移量地址：1E4638
#define PIC_CALL_FUN_ADDRESS 0x1E4638

//恢复地址偏移量,也就是下一个字节
#define PIC_CALL_BACK_ADRESS 0x1E463D

#define HOOK_COMMAND_LEN 5

//发送消息函数的偏移量
#define SEND_CALL_RL_ADDRESS 0x2DA2D7

//发送消息函数的偏移量
#define WECHAT_SEND_MSG_CALL_RL_ADDRESS 0x2DA0F0
//#define WECHAT_SEND_MSG_CALL_RL_ADDRESS 0x2990320

//微信收消息的回调call
#define WECHAT_RECV_MSG_CALL_RL_ADDRESS 0x305753


//数据基址
#define WXADDR_DATA_BASE *((PDWORD)(WECHAT_WIN_BASE+0x113227C))

//群组链表入口
#define WXADDR_ALL_GROUPS (WECHAT_WIN_BASE+0x678+0x64)


//二维码部分
BOOL WeChat_QrCodeHook_Install();
BOOL WeChat_QrCodeHook_UnInstall();

//发送消息，wxid 群聊或者个人的id，message消息id，atid@人的id
void wx_send_text_message(wchar_t* wxId, wchar_t* message, wchar_t* atId);
void wx_send_xml_message(wchar_t* wxid, wchar_t* fWxid, wchar_t* title, wchar_t* content, wchar_t* pic);
void wx_send_share_user_card(wchar_t* wxid, wchar_t* fWxid, wchar_t* name);


#pragma region 结构体部分

struct wx_xml
{
	//标题
	wchar_t* pTitleStr;
	int titleLen;
	int titleMaxLen;
	int titleFill = 0;
	int titleFill2 = 0;
	//url
	wchar_t* pUrlStr;
	int urlLen;
	int urlMaxLen;
	int urlFill = 0;
	int urlFill2 = 0;
	//
};


//3填充
struct wx_str_3
{
	wchar_t* pStr;
	int strLen;
	int strMaxLen;
};

//5个填充
struct wx_str_5
{
	wchar_t* str;
	int len;
	int maxLen;
	int f1 = 0;
	int f2 = 0;
};

struct chat_room_user_info
{
	wchar_t wxid[0x100];
	wchar_t username[0x100];
	wchar_t nickname[0x100];
	wchar_t v1[0x100];
	//是否获取成功
	bool success = FALSE;
};

//微信用户
struct we_chat_user
{
	wchar_t wxid[0x100] = { 0 };
	wchar_t username[0x100] = { 0 };
	wchar_t nickname[0x100] = { 0 };
	wchar_t avatar[0x200]{};
	wchar_t account[0x200]{};
	wchar_t remark[0x200]{};
	wchar_t addType[0x200]{};
	bool success = false;
	int type = 1;
};

/*用户登录信息结构体*/
struct wx_login_user_info
{
	wchar_t * wxid;
	wchar_t * account;
	wchar_t * avatar;
	wchar_t * nickname;
	wchar_t * phone;
	wchar_t * address;
	wchar_t * mobileType;
	wchar_t * qrcode;
};


//聊天消息结构体
struct WECHAT_RECV_MESSAGE
{
	CHAR* wxid = { 0 }; //接受到消息的目标
	CHAR* nickname = { 0 };
	CHAR* content = { 0 }; //消息内容
	int recType = 0; //0:普通用户消息，1:群聊消息
	CHAR* senderWxId = { 0 }; //发送者微信ID，群聊的时候
	CHAR* senderName = { 0 };
};

#pragma endregion

//修改好友备注

//获取群成员列表
std::list<chat_room_user_info> get_chat_room_user_list(wchar_t* chat_room_wid);

// 获取好友列表
std::list<we_chat_user> get_friends_list(int type);

//通过微信id拿到用户信息
chat_room_user_info get_user_info_by_wid(wchar_t* wid);

//确认添加好友


//判断是否已登录
bool check_wx_is_login();

//消息回调函数
typedef void(*RecvMessageCallbackFunc)(WECHAT_RECV_MESSAGE);

cJSON* user_recvmsg_to_json(WECHAT_RECV_MESSAGE msg);

//通用工具包含的库
DWORD GetWechatWinDllAddress();


//获取用户信息
wx_login_user_info wx_get_current_user_info();


//接受消息的Hook
BOOL WeChat_RecvMessage_Hook_Install(RecvMessageCallbackFunc recvCallFun);
BOOL WeChat_RecvMessage_Hook_UnInstall();


//系统状态
bool WeChatStateChangeHookInstall(DWORD callback);


//关闭当前系统进程
void close_current_process();


wchar_t* UTF8ToUnicode(const char* str);
char* UnicodeToUTF8(const WCHAR* str);


//utf8 转GBK
char* Tool_GbkToUtf8(const char* src_str);
char* Tool_UTF8ToGBK(const char* src_str);

wchar_t* Tool_CharToWchar(const char* c, size_t m_encode);
//CP_ACP
char* Tool_WcharToChar(const wchar_t* wp, size_t m_encode);
char * UnicodeToANSI(const wchar_t *str);
