// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <list>
#include <stdio.h>
#include <combaseapi.h>
#define SERVICE
#include "../api.h"
#include "../plugin_id.h"
#define DEFAULT_PORT "27015"

HANDLE handle_event = NULL, handle_thread = NULL;
DWORD dwFilter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;
HKEY hKey;
DWORD type = REG_SZ;


typedef struct _DllListElement
{
	GUID pluginId;
	const wchar_t* DllName;
	pfn_GetCopyright GetCopyright;
} DllListElement;

std::list<DllListElement> g_DllList = {
	{ plugin1_id, L"Plugin1_1.dll", nullptr },
	{ plugin2_id, L"Pugin2.dll", nullptr }
};


long registry_reading() {
	// Reading
	DWORD cValues;              // number of values for key 
	DWORD cbMaxValueData;       // longest value data 
	DWORD retCode;
	DWORD cchValue = 16383;
	wchar_t guidString[512];
	wchar_t dllName[512];

	retCode = RegQueryInfoKey(
		hKey,            // key handle 
		NULL,            // buffer for class name 
		NULL,            // size of class string 
		NULL,            // reserved 
		NULL,            // number of subkeys 
		NULL,            // longest subkey size 
		NULL,            // longest class string 
		&cValues,        // number of values for this key 
		NULL,            // longest value name 
		&cbMaxValueData, // longest value data 
		NULL,            // security descriptor 
		NULL);

	for (DWORD i = 0, retCode = ERROR_SUCCESS; i < cValues; i++) {
		cchValue = 16383;
		guidString[0] = L'\0';

		DWORD lpData = cbMaxValueData;
		dllName[0] = L'\0';

		retCode = RegEnumValueW(hKey, i,
			guidString,
			&cchValue,
			NULL,
			NULL,
			(LPBYTE)dllName,
			&lpData);

		// Adding new elements to list
		if (retCode == ERROR_SUCCESS) {
			IID guid;
		
			// Make GUID from string
			const auto res2 = IIDFromString(guidString, &guid);
			if (res2 != S_OK) {
				printf("Enable to convert %S to GUID \n", guidString);
				return NULL;
			}

			// Updating 
			bool upd = false;
			for (auto i = begin(g_DllList); i != end(g_DllList); i++)
			{
				auto element = *i;
				if (IsEqualGUID(element.pluginId, guid))
				{
					*i = DllListElement({ guid, dllName, nullptr });
					printf("Plugin with GUID %S was updated with new dll: %S\n", guidString, dllName);
					upd = true;
					break;
				}
			}
			// If this dll is new add new instance to list
			if (!upd) {
				g_DllList.push_back(DllListElement({ guid, dllName, nullptr }));
				printf("Plugin with GUID %S was added with new dll: %S\n", guidString, dllName);
			}
		}
	}
	return 0;
}


DWORD WINAPI update_thread(LPVOID lpParam) {
	DWORD dwWaitResult;

	while (true) {
		dwWaitResult = WaitForSingleObject(handle_event, INFINITE);
		switch (dwWaitResult) {
		case WAIT_OBJECT_0: {
			registry_reading();
			//reissue the notify command
			RegNotifyChangeKeyValue(hKey, TRUE, dwFilter, handle_event, TRUE);
			break;
		}
		default:
			return 0;
		}
	}
	return 1;
}

SOCKET server_init(void)
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	struct addrinfo *result = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Create a SOCKET for connecting to server
	const SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	freeaddrinfo(result);
	//closesocket(listenSocket);	//this should be done by calling server_stop
	//WSACleanup();					//this should be done by calling server_done
	return listenSocket;
}

void server_stop(const SOCKET listenSocket)
{
	closesocket(listenSocket);
}

void server_done(void)
{
	WSACleanup();
}

SOCKET server_listen(const SOCKET listenSocket)
{
	const int iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// Accept a client socket
	const SOCKET clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	return clientSocket;
}

const char* GetPluginCopyright(GUID* guid);

int server_process_request(const SOCKET clientSocket)
{
    #define MAX_BUFFER 1024
	int iResult;
	char recvbuf[MAX_BUFFER];
	const int recvbuflen = sizeof(recvbuf);
	char sendbuf[MAX_BUFFER] = "Copyrigth Server 2000-2018";
	int sendbuflen = sizeof(sendbuf);

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(clientSocket, (char*)&recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			if (iResult != 16)
			{
				printf("Invalid send buffer length \n");
			}
			else
			{
				auto ret = GetPluginCopyright((GUID*)recvbuf);
				memcpy(sendbuf, ret, strlen(ret) + 1); //TODO check overflow
				sendbuflen = strlen(ret) + 1;
			}

			const int iSendResult = send(clientSocket, (char*)&sendbuf, sendbuflen, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(clientSocket);
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else  {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			return 1;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		return 1;
	}

	// cleanup
	closesocket(clientSocket);
	return 0;
}

int server(int argc, char **argv)
{
	const SOCKET listenSocket = server_init();
	if (listenSocket == INVALID_SOCKET) return 1;

	while (1)
	{
		const SOCKET clientSocket = server_listen(listenSocket);
		if (clientSocket == INVALID_SOCKET) break;
		server_process_request(clientSocket);
	}

	server_stop(listenSocket);
	server_done();
	return 0;
}


const char* GetPluginCopyright(GUID* guid)
{
	// TODO: check if library already loaded in memory
	printf("GetPluginCopyright: searching... \n");
	for (DllListElement element : g_DllList)
	{
		if (IsEqualGUID(element.pluginId, *guid))
		{
			printf("GetPluginCopyright: Guid was found \n");
			if (nullptr == element.GetCopyright)
			{
				//Resolve
				printf("GetPluginCopyright: Dll %S is loading \n", element.DllName);
				auto hLibrary = LoadLibrary(element.DllName);
				if (hLibrary)
				{
					printf("GetPluginCopyright: Dll %S was loaded \n", element.DllName);
					void* pfn = GetProcAddress(hLibrary, "GetCopyright");
					printf("GetPluginCopyright: _GetCopyright at %p \n", pfn);
					element.GetCopyright =
						reinterpret_cast<pfn_GetCopyright>(pfn);
				}
			}
			if (nullptr == element.GetCopyright) break;

			return element.GetCopyright();
		}
	}
	return "Error";
}

int main(int argc, char* argv[])
{
	
	DWORD num_thread;
	LPCTSTR strKeyName = L"Software\\Task2";
	
	printf("Starting server\n");
	// Open the key for PluginList and PerfMon 
	long sts = RegCreateKeyEx(HKEY_CURRENT_USER, strKeyName, NULL, NULL, 
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	// Runing thread
	if (sts == ERROR_SUCCESS) {
		printf("Registry key %S was opened successfully\n", strKeyName);
		handle_event = CreateEvent(NULL, TRUE, FALSE, L"Register Event");
		handle_thread = CreateThread(NULL, 0, update_thread, NULL, 0, &num_thread);

		RegNotifyChangeKeyValue(hKey, TRUE, dwFilter, handle_event, TRUE);
	}


	server(argc, argv);
	return 0;
}

/*
#else
HMODULE hLibrary = LoadLibrary(L"TransportDll.dll");
if (hLibrary == nullptr)
{
	printf("LoadLibrary failed %lu \n", GetLastError());
	return -1;
}

void* p = (void*)GetProcAddress(hLibrary, "GetCopyright");
if (nullptr == p)
{
	printf("GetProcAddress failed %lu \n", GetLastError());
	return -1;
}
GetCopyright = reinterpret_cast<pfn_GetCopyright>(p);
printf("Copyright = %s \n", GetCopyright(plugin1_id));
#endif
*/