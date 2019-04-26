#include "stdafx.h"
#include "wx_helper.h"
#include <atlimage.h>
#include <direct.h>


#define CWD_BUFFER_LEN 0x3000

char* GetCurrentCwd()
{
	char buffer[CWD_BUFFER_LEN];
	if (_getcwd(buffer, CWD_BUFFER_LEN))
	{
	}
	return buffer;
}


//保存二维码数据
VOID SaveQrCodeImg(DWORD qrcode)
{
	//图片长度
	DWORD picLen = qrcode + 0x4;
	//图片的数据,4M
	char PicData[0xFFF] = {0};
	size_t cpyLen = (size_t)*((LPVOID *)picLen);
	//获取到图片数据了
	memcpy(PicData, *((LPVOID *)qrcode), cpyLen);
	FILE* pFile;

	//打开文件 不存在就创建
	char path[0x3000];
	sprintf_s(path, "%s%s", GetCurrentCwd(), "\\qr-code.png");
	fopen_s(&pFile, path, "wb");

	//图片数据
	fwrite(PicData, sizeof(char), sizeof(PicData), pFile);
	//关闭
	fclose(pFile);
}

//声明寄存器
//pushad
//pushsd
//popad
//popsd
DWORD pEax = 0;
DWORD pEcx = 0;
DWORD pEdx = 0;
DWORD pEbx = 0;
DWORD pEbp = 0;
DWORD pEsp = 0;
DWORD pEsi = 0;
DWORD pEdi = 0;
//生命一个裸函数，不影响堆栈,不做其他多余的操作，告诉编译器,裸函数不能再内部神秘变量
DWORD _QrCodeRetAdd = 0;
VOID __declspec(naked) WeChat_QrCodeHook_ShowPic()
{
	////备份寄存器
	__asm {
		mov pEax, eax
		mov pEcx, ecx
		mov pEdx, edx
		mov pEbx, ebx
		mov pEbp, ebp
		mov pEsp, esp
		mov pEsi, esi
		mov pEdi, edi
	}
	//我们的二维码数据在ecx里面
	SaveQrCodeImg(pEcx); //我们现在写一个函数用来保存二维码数据
	//////返回的地址 1E463D
	_QrCodeRetAdd = GetWechatWinDllAddress() + PIC_CALL_BACK_ADRESS;
	__asm {
		mov eax, pEax
		mov ecx, pEcx
		mov edx, pEdx
		mov ebx, pEbx
		mov ebp, pEbp
		mov esp, pEsp
		mov esi, pEsi
		mov edi, pEdi
		jmp _QrCodeRetAdd
	}
}

//备份的5个字节
BYTE buf[HOOK_COMMAND_LEN] = {0}; //是需要5个字节的数据就可以了

//把内存地址中的信息替换成我们的DLL函数,hook
BOOL WeChat_QrCodeHook_Install()
{
	DWORD funShow = (DWORD)(&WeChat_QrCodeHook_ShowPic); //取函数地址

	DWORD dw = GetWechatWinDllAddress(); //获取dll基址
	DWORD hookCall = dw + PIC_CALL_FUN_ADDRESS; //hook的函数真实地址

	//E8 D34D2900    E8代表call，后面的数据值= 要跳转的地址-hook的地址-5个字节
	BYTE jmpCode[HOOK_COMMAND_LEN] = {0}; //需要替换掉的指令代码
	jmpCode[0] = 0xE9; //jmp指令
	*(DWORD *)&jmpCode[1] = (DWORD)funShow - hookCall - HOOK_COMMAND_LEN; //获取地址的值

	//1.备份原来的数据,以后卸载hook需要用得到
	HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
	if (FALSE == ReadProcessMemory(hd, (LPCVOID)hookCall, buf, sizeof(buf), NULL))
	{
		return FALSE;
	}

	//2.写入我们的自己的指令
	if (FALSE == WriteProcessMemory(hd, (LPVOID)hookCall, jmpCode, HOOK_COMMAND_LEN, NULL))
	{
		return FALSE;
	}
	OutputDebugStringA("安装QrCodeHook成功");
	//关闭句柄
	CloseHandle(hd);
	return TRUE;
}


BOOL WeChat_QrCodeHook_UnInstall()
{
	//恢复HOOK

	DWORD hookCallAddr = GetWechatWinDllAddress() + PIC_CALL_FUN_ADDRESS;
	HANDLE hd = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
	if (FALSE == WriteProcessMemory(hd, (LPVOID)hookCallAddr, buf, HOOK_COMMAND_LEN, NULL))
	{
		return FALSE;
	}
	//关闭句柄
	CloseHandle(hd);
	OutputDebugStringA("已卸载QrCodeHook");
	return TRUE;
}
