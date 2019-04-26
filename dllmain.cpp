#include "stdafx.h"
#include "wx_helper.h"
#include <Windows.h>
#include <src/sio_client.h>
#include <iostream>
#include <mutex>
#include <atlconv.h>
#include <debugapi.h>

using namespace std;
using namespace sio;

DWORD WINAPI ThreadProc(_In_ HMODULE hModule);


//互斥锁
std::mutex _lock;

//config
#define WX_SERVER_ADDRESS "http://127.0.0.1:3008/"
#define WX_SERVER_QUERY "token=we_chat&client_type=we_chat"
sio::client _weChatWsClient;

bool connect_finish = false;

bool ack_user_login_next(sio::event& ev)
{
	if (0 == check_wx_is_login())
	{
		auto mm = object_message::create();
		mm->get_map()["code"] = int_message::create(401);
		mm->get_map()["message"] = string_message::create("please login first");
		ev.put_ack_message(message::list(mm));
		return false;
	}
	return true;
}


//socket事件链接的socket
class connection_listener
{
	sio::client& handler;

public:

	connection_listener(sio::client& h) : handler(h)
	{
	}


	void on_connected()
	{
		_lock.lock();
		connect_finish = true;
		OutputDebugStringA("服务端连接成功");
		_lock.unlock();
	}

	void on_close(client::close_reason const& reason)
	{
		OutputDebugStringA("服务端断开链接");
		close_current_process();
	}

	void on_fail()
	{
		std::cout << "sio failed " << std::endl;
	}
};

/**
 * 接收到消息
 */
VOID RecMessageCallback(WECHAT_RECV_MESSAGE message)
{
	_lock.lock();
	const auto res = object_message::create();
	res->get_map()["type"] = int_message::create(100);

	const auto msg = object_message::create();
	msg->get_map()["wxid"] = string_message::create(message.wxid);
	msg->get_map()["nickname"] = string_message::create(message.nickname);
	msg->get_map()["sender_wxid"] = string_message::create(message.senderWxId);
	msg->get_map()["sender_name"] = string_message::create(message.senderName);
	msg->get_map()["type"] = int_message::create(message.recType);
	msg->get_map()["content"] = string_message::create(message.content);

	res->get_map()["data"] = msg;
	_weChatWsClient.socket()->emit("wx.message", res);
	_lock.unlock();
}


//包装所有的事件
void wx_client_wrapper(const socket::ptr sock)
{
	//群成员
	sock->on("wx.get_room_users", [&](sio::event& ev)
	{
		_lock.lock();
		if (!ack_user_login_next(ev)) {
			_lock.unlock();
			return;
		}
		const auto data = ev.get_message();
		const string wxid = data->get_map()["wxid"]->get_string();
		wchar_t* id = UTF8ToUnicode(wxid.c_str());
		std::list<chat_room_user_info> list = get_chat_room_user_list(id);

		const auto res = object_message::create();
		res->get_map()["code"] = int_message::create(200);
		res->get_map()["message"] = string_message::create("success");
		const auto users = array_message::create();

		for (auto it = list.begin(); it != list.end(); ++it)
		{
			const auto user = object_message::create();
			user->get_map()["wxid"] = string_message::create(UnicodeToUTF8(it->wxid));
			user->get_map()["username"] = string_message::create(UnicodeToUTF8(it->username));
			user->get_map()["nickname"] = string_message::create(UnicodeToUTF8(it->nickname));
			user->get_map()["v1"] = string_message::create(UnicodeToUTF8(it->v1));
			user->get_map()["success"] = bool_message::create(it->success);
			users->get_vector().push_back(user);
		}
		res->get_map()["data"] = users;
		ev.put_ack_message(res);
		_lock.unlock();
	});
	//获取所有的好友
	sock->on("wx.get_users", [&](sio::event& ev)
	{
		_lock.lock();
		if (!ack_user_login_next(ev)) {
			_lock.unlock();
			return;
		}
		wx_login_user_info info = wx_get_current_user_info();
		std::list<we_chat_user> list = get_friends_list(1);
		const auto res = object_message::create();
		res->get_map()["code"] = int_message::create(200);
		res->get_map()["message"] = string_message::create("success get users");
		const auto users = array_message::create();
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			const auto user = object_message::create();
			std::string wxid = UnicodeToUTF8(it->wxid);
			user->get_map()["wxid"] = string_message::create(wxid);
			//获取类型
			it->type = 2; //用户
			if (string::npos != wxid.find("chatroom"))
			{
				it->type = 3; //群
			}
			if (string::npos != wxid.find("gh_"))
			{
				it->type = 4; //公众号
			}
			if (wxid.compare("fmessage") == 0)
			{
				it->type = 5; //
			}
			if (wxid.compare("floatbottle") == 0)
			{
				it->type = 5; //
			}
			if (wxid.compare("medianote") == 0)
			{
				it->type = 5; //
			}
			if (wxid.compare("newsapp") == 0)
			{
				it->type = 5; //
			}
			if (wxid.compare("weixinguanhaozhushou") == 0)
			{
				it->type = 5; //
			}
			if (wxid.compare("qmessage") == 0)
			{
				it->type = 5; //
			}
			if (wxid.compare("mphelper") == 0)
			{
				it->type = 5; //
			}
			auto info = get_user_info_by_wid(it->wxid);
			auto account = it->account;
			auto fuckId = it->wxid;
			if (wcslen(info.username) > 0) {
				account = info.username;
				fuckId = info.wxid;
			}
			user->get_map()["account"] = string_message::create(UnicodeToUTF8(account));
			user->get_map()["username"] = string_message::create(UnicodeToUTF8(account));
			user->get_map()["nickname"] = string_message::create(UnicodeToUTF8(it->nickname));
			user->get_map()["remark"] = string_message::create(UnicodeToUTF8(it->remark));
			user->get_map()["avatar"] = string_message::create(UnicodeToUTF8(it->avatar));
			user->get_map()["success"] = bool_message::create(it->success);
			user->get_map()["type"] = int_message::create(it->type);
			user->get_map()["fuckid"] = string_message::create(UnicodeToUTF8(fuckId));

			users->get_vector().push_back(user);
		}
		res->get_map()["data"] = users;
		ev.put_ack_message(res);
		_lock.unlock();
	});
}


message::ptr get_login_user_obj() {
	wx_login_user_info info = wx_get_current_user_info();

	const auto res = object_message::create();
	res->get_map()["code"] = int_message::create(200);
	const auto userInfo = object_message::create();

	//info.nickname
	userInfo->get_map()["wxid"] = string_message::create(UnicodeToUTF8(info.wxid));
	userInfo->get_map()["account"] = string_message::create(UnicodeToUTF8(info.account));
	userInfo->get_map()["nickname"] = string_message::create(UnicodeToUTF8(info.nickname));
	userInfo->get_map()["avatar"] = string_message::create(UnicodeToUTF8(info.avatar));
	userInfo->get_map()["address"] = string_message::create(UnicodeToUTF8(info.address));
	userInfo->get_map()["phone"] = string_message::create(UnicodeToUTF8(info.phone));
	userInfo->get_map()["mobile_type"] = string_message::create(UnicodeToUTF8(info.mobileType));
	res->get_map()["data"] = userInfo;

	return res;
}

//链接WebSocker服务器
void wx_connect_server()
{
	connection_listener l(_weChatWsClient);
	_weChatWsClient.set_open_listener(std::bind(&connection_listener::on_connected, &l));
	_weChatWsClient.set_close_listener(std::bind(&connection_listener::on_close, &l, std::placeholders::_1));
	_weChatWsClient.set_fail_listener(std::bind(&connection_listener::on_fail, &l));

	std::map<string, string> map;
	map["token"] = "wx";
	map["client_type"] = "wx";
	_lock.lock();
	if (!connect_finish)
	{
		//_cond.wait(_lock);
	}
	_lock.unlock();

	_weChatWsClient.socket()->on("disconnect", [&](sio::event& ev)
	{
		connect_finish = false;
		OutputDebugStringA("disconnect server success");
	});

	_weChatWsClient.socket()->on("connect", [&](sio::event& ev)
	{
		connect_finish = true;
		OutputDebugStringA("connect server success");
	});

	_weChatWsClient.socket()->on("pong", [&](sio::event& ev)
	{
		_lock.lock();
		auto mm = object_message::create();
		mm->get_map()["code"] = int_message::create(200);
		const std::string msg = "pong ack";
		mm->get_map()["message"] = string_message::create(msg);
		ev.put_ack_message(mm);
		_lock.unlock();
	});
	//自杀
	_weChatWsClient.socket()->on("wx.kill", [&](sio::event& ev)
	{
		close_current_process();
	});
	//退出
	_weChatWsClient.socket()->on("wx.exit", [&](sio::event& ev)
	{
		close_current_process();
	});

	//执行获取微信用户的信息数据
	_weChatWsClient.socket()->on("wx.get_login_user", [&](sio::event& ev)
	{
		//if (!ack_user_login_next(ev))return;
		_lock.lock();
		//查询内存信息，然后包装数据返回
		const auto res = get_login_user_obj();
		ev.put_ack_message(res);
		_lock.unlock();
	});


	//执行发送消息,JSON格式如下：type=[person,group,gh],wxid,message,atWxid
	_weChatWsClient.socket()->on("wx.send_message", [&](sio::event& ev)
	{
		_lock.lock();
		if (!ack_user_login_next(ev)) {
			_lock.unlock();
			return;
		}
		const auto data = ev.get_message();
		const string msgType = data->get_map()["type"]->get_string();
		const string wxid = data->get_map()["wxid"]->get_string();
		const string message = data->get_map()["message"]->get_string();
		const string atid = data->get_map()["atid"]->get_string(); //at人的微信ID

		//char 默认是gbk 需要转成utf8,转宽字符
		wchar_t* mt = UTF8ToUnicode(msgType.c_str());
		wchar_t* id = UTF8ToUnicode(wxid.c_str());
		wchar_t* msg = UTF8ToUnicode(message.c_str());
		wchar_t* at = UTF8ToUnicode(atid.c_str());
		wx_send_text_message(id, msg, at);

		//包装消息进行发送
		auto mm = object_message::create();
		mm->get_map()["code"] = int_message::create(200);
		mm->get_map()["status"] = bool_message::create(true);
		mm->get_map()["message"] = string_message::create(message);
		ev.put_ack_message(message::list(mm));

		_lock.unlock();
	});

	//包装其他所有的事件
	const socket::ptr ptr = _weChatWsClient.socket();
	wx_client_wrapper(ptr);

	//发送名片
	ptr->on("wx.send_card_message", [&](sio::event& ev)
	{
		if (!ack_user_login_next(ev))return;
		message::ptr data = ev.get_message();
		string wxId = data->get_map()["wxid"]->get_string();
		string cardWxId = data->get_map()["card_wx_id"]->get_string();
		string name = data->get_map()["name"]->get_string();
		wchar_t* mt = UTF8ToUnicode(wxId.c_str());
		wchar_t* id = UTF8ToUnicode(cardWxId.c_str());
		wchar_t* nm = UTF8ToUnicode(name.c_str());
		wx_send_share_user_card(mt, id, nm);
	});
	//发送xml普通消息
	ptr->on("wx.send_xml_message", [&](sio::event& ev)
	{
		if (!ack_user_login_next(ev))return;
		message::ptr data = ev.get_message();
		string wxId = data->get_map()["wxid"]->get_string();
		string cardWxId = data->get_map()["for_wxid"]->get_string();
		string title = data->get_map()["title"]->get_string();
		string content = data->get_map()["content"]->get_string();
		string pic = data->get_map()["pic"]->get_string();

		wchar_t* mt = UTF8ToUnicode(wxId.c_str());
		wchar_t* id = UTF8ToUnicode(cardWxId.c_str());
		wchar_t* tl = UTF8ToUnicode(title.c_str());
		wchar_t* ct = UTF8ToUnicode(content.c_str());
		wchar_t* pc = UTF8ToUnicode(pic.c_str());
		wx_send_xml_message(mt, id, tl, ct, pc);
	});
	//发送语音消息
	ptr->on("wx.send_voice_message", [&](sio::event& ev)
	{
		message::ptr data = ev.get_message();
	});
	//发送图片消息
	ptr->on("wx.send_image_message", [&](sio::event& ev)
	{
		message::ptr data = ev.get_message();
		string wxid = data->get_map()["wxid"]->get_string();
		string message = data->get_map()["message"]->get_string();
		string atid = data->get_map()["atid"]->get_string(); //at人的微信ID
	});
	_weChatWsClient.connect(WX_SERVER_ADDRESS, map);
}

//install all hooker listneners
VOID inject_all_hook_listeners(HWND hwndDlg)
{
	INT hookerCount = 0;
	if (TRUE == WeChat_QrCodeHook_Install())
	{
		hookerCount++;
	}
}

/*状态变动的回调事件*/
VOID wx_state_change_callback(DWORD state) {
	if (state == 0) {
		_lock.lock();
		if (connect_finish == true) {
			const auto  msg = object_message::create();
			msg->get_map()["code"] = int_message::create(401);
			msg->get_map()["message"] = string_message::create("wechat logout");
			_weChatWsClient.socket()->emit("wx.logout", msg);
		}
		Sleep(3 * 1000);
		_lock.unlock();
		//注销 直接干掉进程
		close_current_process();
	}
	else {
		_lock.lock();
		//登录成功
		if (connect_finish == true) {
			const auto res = get_login_user_obj();
			_weChatWsClient.socket()->emit("wx.login", res);
		}
		_lock.unlock();
	}
}

DWORD WINAPI ThreadProc(_In_ HMODULE hModule)
{
	wx_connect_server(); //进行服务器的链接

	OutputDebugString(L"开始安装钩子");
	////1.qr-code
	WeChat_QrCodeHook_Install();

	////2.接受消息
	const auto func = RecMessageCallback;
	WeChat_RecvMessage_Hook_Install(func); //需要传入一个回调函数

	//3.安装登录状态钩子
	const auto scb = (DWORD)&wx_state_change_callback;
	WeChatStateChangeHookInstall(scb);

	//安装所有的Hook
	return TRUE;
}

long __stdcall MyUnhandledExceptionFilter(EXCEPTION_POINTERS* exp)
{
	MessageBoxA(0, "Error", "error", MB_OK);
	return EXCEPTION_EXECUTE_HANDLER;
}

//启动dll回调事件
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	HANDLE tdh = nullptr;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//创建线程去执行
		SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
		tdh = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(ThreadProc), hModule, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		//程序关闭后需要释放一下socket
		_weChatWsClient.sync_close();
		_weChatWsClient.clear_con_listeners();
		CloseHandle(tdh);
		OutputDebugString(L"微信程序退出");
		close_current_process();
		break;
	default:
		break;
	}
	return TRUE;
}