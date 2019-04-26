#include "stdafx.h"
#include <stdio.h>
#include "malloc.h"
#include "resource.h"
#include  "GetWxInfo.h"
#include "WeChatHelper.h"

//发送消息的库

struct WeChatTextMessage
{
	wchar_t *pStr;//消息
	int strLen;//内容的长度
	int iStrLen;//最后的文本就是wxid*2
};


//1015A2D0    53              push ebx
//1015A2D1    8D8D ECFDFFFF   lea ecx, dword ptr ss : [ebp - 0x214]
//1015A2D7    E8 F49BD7FF     call WeChatWi.0FED3ED0
//2. 在一片空白区域创建自己的字符串
//11767D24  11317048  UNICODE "eee"  文本内容
//11767D28  00000003  长度
//11767D2C  00000004  填写长度的2倍
//
//3.修改堆栈地址和修改ebx的地址，发送
//发送调用代码如下
//0FF443F8    8B55 CC         mov edx, dword ptr ss : [ebp - 0x34]
//0FF443FB    8D43 14         lea eax, dword ptr ds : [ebx + 0x14]
//0FF443FE    6A 01           push 0x1  第一个参数 0x1
//0FF44400    50              push eax	第二个参数eax
//0FF44401    53              push ebx 第三个参数ebx 就是发送消息的内容
//0FF44402    8D8D E4F7FFFF   lea ecx, dword ptr ss : [ebp - 0x81C]
//0FF44408    E8 E35C2100     call WeChatWi.1015A0F0
//0FF4440D    83C4 0C         add esp, 0xC

///atid 就是被at人的微信ID
BOOL WeChat_SendTextMessage(wchar_t* wxid, wchar_t* message, wchar_t* atid) {

	//1.构造发送的人
	WeChatTextMessage recv = { 0 };
	recv.pStr = wxid;
	recv.strLen = wcslen(wxid);
	recv.iStrLen = 2 * recv.strLen;

	//2.构造消息
	WeChatTextMessage msg = { 0 };
	msg.pStr = message;
	msg.strLen = wcslen(message);
	msg.iStrLen = 2 * msg.strLen;


	//3.构造群聊At人的消息
	WeChatTextMessage atRecv = { 0 };
	atRecv.pStr = atid;
	atRecv.strLen = wcslen(atid);
	atRecv.iStrLen = 2 * msg.strLen;

	//获取数据
	char* asmWxId = (char*)&recv.pStr;//这个直接是地址
	char* asmMsg = (char*)&msg.pStr;//消息
	char* asmAt = (char*)atRecv.pStr;//at的人

	char buff[0x81C] = { 0 };

	//call的地址
	DWORD sendCallAddr = GetWechatWinDllAddress() + WECHAT_SEND_MSG_CALL_RL_ADDRESS;

	//先检测是不是一个正确的call地址
	char txt[0x100] = { 0 };
	sprintf_s(txt, "WXID=%s,消息内容=%s,艾特=%s", wxid, message, atid);
	OutputDebugStringA(txt);
	//执行汇编,eax一直保持是0
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
	return TRUE;
}

//
VOID WeChat_SendImageMessage(wchar_t * wxid, BYTE *picPath) {


}

VOID On_RecvMessage() {

}

//hook收消息地址 安装钩子函数
VOID WeChat_RecvMessage_Hook_Install(DWORD hookCallAddr) {





}
//卸载钩子函数
VOID WeChat_RecvMessage_Hook_UnInstall() {

}