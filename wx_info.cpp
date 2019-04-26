// GetWxInfo.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <cstdio>
#include "wx_helper.h"
#include <malloc.h>
#include <Windows.h>
#include "cJSON.h"
#include <atlstr.h>
#include <string>


using namespace std;

//获取
cJSON* user_recvmsg_to_json(WECHAT_RECV_MESSAGE message)
{
	/* 创建空Student JSON对象 */
	cJSON* jbj = cJSON_CreateObject();

	cJSON_AddItemToObject(jbj, "wxid", cJSON_CreateString(message.wxid)); //根节点下添加
	cJSON_AddItemToObject(jbj, "nickname", cJSON_CreateString(message.nickname)); //根节点下添加
	cJSON_AddItemToObject(jbj, "senderId", cJSON_CreateString(message.senderWxId)); //根节点下添加
	cJSON_AddItemToObject(jbj, "senderName", cJSON_CreateString(message.senderName)); //根节点下添加
	cJSON_AddItemToObject(jbj, "type", cJSON_CreateNumber(message.recType)); //根节点下添加
	cJSON_AddItemToObject(jbj, "message", cJSON_CreateString(message.content)); //根节点下添加

	/* 返回Student结构体对象指针 */
	return jbj;
}

//获取wechatdll基址
DWORD GetWechatWinDllAddress()
{
	HMODULE win_add = LoadLibraryA("WeChatWin.dll");
	return reinterpret_cast<DWORD>(win_add);
}

//ll基址 52AE0000
//WeChatWin.dll + 1131DC8   昵称地址
//WeChatWin.dll + 1131F2C   头像地址
//WeChatWin.dll + 1131C98   手机号码
//WeChatWin.dll + 1131DC8   微信ID
//WeChatWin.dll + 1132030   机型
//WeChatWin.dll + 1131D68   地区
//读取内存数据
wx_login_user_info wx_get_current_user_info()
{
	DWORD WINDLL_DW_ADDRESS = GetWechatWinDllAddress(); //获取基地址
	DWORD wxidAddr = WINDLL_DW_ADDRESS + 0x1131B90;

	//wxid 微信id的地址
	DWORD wxidAddr_2 = WINDLL_DW_ADDRESS + 0x1131B78;

	//1131DC8 微信username地址
	DWORD wxnoAddr = WINDLL_DW_ADDRESS + 0x1131DC8;

	DWORD WX_AVATAR_ADRESS = WINDLL_DW_ADDRESS + 0x1131F2C;
	DWORD WX_PHONE_ADRESS = WINDLL_DW_ADDRESS + 0x1131C98;

	DWORD WX_NICKNAME_ADRESS = WINDLL_DW_ADDRESS + 0x1131C64;

	DWORD WX_ADD_ADRESS = WINDLL_DW_ADDRESS + 0x1131D68;
	DWORD WX_QRSTR_ADRESS = WINDLL_DW_ADDRESS + 0x114658C; //二维码
	DWORD wx_mobile_type_addr = WINDLL_DW_ADDRESS + 0x1102218; //设备
	wx_login_user_info info = { 0 };

	info.wxid = UTF8ToUnicode((const char*)wxidAddr_2);
	if (wcslen(info.wxid) < 0x6) {
		DWORD pWxid = WINDLL_DW_ADDRESS + 0x1131B78;
		pWxid = *((DWORD *)pWxid);
		info.wxid = UTF8ToUnicode((const char *)pWxid);
	}
	info.account = UTF8ToUnicode((const char*)wxnoAddr);
	//info.nickname = UTF8ToUnicode();
	//auto nick = (const char *)WX_NICKNAME_ADRESS;
	/*auto len = strlen(nick);
	if (len == 4) {
		DWORD pna = WX_NICKNAME_ADRESS + 0x0;
		pna = *((DWORD *)pna);
		auto st = (const char *)pna;
		nick = st;
	}*/
	auto my = get_user_info_by_wid(info.wxid);
	if (wcslen(my.nickname) > 0) {
		char* name = UnicodeToUTF8(my.nickname);
		info.nickname = UTF8ToUnicode(name);
	}
	else {
		info.nickname = UTF8ToUnicode("");
	}
	info.phone = UTF8ToUnicode((const char*)WX_PHONE_ADRESS);
	info.address = UTF8ToUnicode((const char*)WX_ADD_ADRESS);
	info.phone = UTF8ToUnicode((const char*)WX_PHONE_ADRESS);
	info.mobileType = UTF8ToUnicode((const char*)wx_mobile_type_addr);

	DWORD rl = *reinterpret_cast<DWORD *>(WX_AVATAR_ADRESS); //获取真实的地址
	info.avatar = UTF8ToUnicode((const char*)rl);

	DWORD qrd = *reinterpret_cast<DWORD *>(WX_QRSTR_ADRESS); //二维码的虚拟地址
	info.qrcode = UTF8ToUnicode((const char*)qrd);
	return info;
}

bool check_wx_is_login()
{
	wx_login_user_info info = wx_get_current_user_info();
	return 0 != wcscmp(info.wxid, L"");
}

VOID test_change_user_info_memory()
{
	DWORD WINDLL_DW_ADDRESS = GetWechatWinDllAddress(); //获取基地址
	DWORD WX_WXID_ADRESS = WINDLL_DW_ADDRESS + 0x1131B90;
	DWORD WX_AVATAR_ADRESS = WINDLL_DW_ADDRESS + 0x1131F2C;
	DWORD WX_PHONE_ADRESS = WINDLL_DW_ADDRESS + 0x1131C98;
	DWORD WX_NICKNAME_ADRESS = WINDLL_DW_ADDRESS + 0x1131C64;
	DWORD WX_ADD_ADRESS = WINDLL_DW_ADDRESS + 0x1131D68;


	//修改内存信息
	if (WriteProcessMemory(GetCurrentProcess(), (LPVOID)WX_NICKNAME_ADRESS, "HelleWorld", sizeof("HelleWorld"), NULL))
	{
		MessageBoxW(NULL, L"修改成功", L"信息", 0);
	}
}


//转码
wchar_t* UTF8ToUnicode(const char* str)
{
	int textlen = 0;
	wchar_t* result;
	textlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	result = (wchar_t *)malloc((textlen + 1) * sizeof(wchar_t));
	memset(result, 0, (textlen + 1) * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)result, textlen);
	return result;
}

//转码utf8
char* UnicodeToUTF8(const WCHAR* str)
{
	int nByte;
	char* zFilename;
	nByte = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	zFilename = static_cast<char*>(malloc(nByte));
	if (zFilename == 0)
	{
		return 0;
	}
	nByte = WideCharToMultiByte(CP_UTF8, 0, str, -1, zFilename, nByte, 0, 0);
	if (nByte == 0)
	{
		free(zFilename);
		zFilename = 0;
	}
	return zFilename;
}
char * UnicodeToANSI(const wchar_t *str)
{
	char * result;
	int textlen = 0;
	textlen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	result = (char *)malloc((textlen + 1) * sizeof(char));
	memset(result, 0, sizeof(char) * (textlen + 1));
	WideCharToMultiByte(CP_ACP, 0, str, -1, result, textlen, NULL, NULL);
	return result;
}

char* Tool_GbkToUtf8(const char* src_str)
{
	int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	char* strTemp = str;
	if (wstr) delete[] wstr;
	if (str) delete[] str;
	return strTemp;
}

char* Tool_UTF8ToGBK(const char* src_str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
	wchar_t* wszGBK = new wchar_t[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	char* strTemp(szGBK);
	if (wszGBK) delete[] wszGBK;
	if (szGBK) delete[] szGBK;
	return strTemp;
}


wchar_t* Tool_CharToWchar(const char* c, size_t m_encode)
{
	wchar_t* str;
	int len = MultiByteToWideChar(m_encode, 0, c, strlen(c), NULL, 0);
	wchar_t* m_wchar = new wchar_t[len + 1];
	MultiByteToWideChar(m_encode, 0, c, strlen(c), m_wchar, len);
	m_wchar[len] = '\0';
	str = m_wchar;
	delete m_wchar;
	return str;
}

char* Tool_WcharToChar(const wchar_t* wp, size_t m_encode)
{
	char* str;
	int len = WideCharToMultiByte(m_encode, 0, wp, wcslen(wp), NULL, 0, NULL, NULL);
	char* m_char = new char[len + 1];
	WideCharToMultiByte(m_encode, 0, wp, wcslen(wp), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	str = m_char;
	delete m_char;
	return str;
}
