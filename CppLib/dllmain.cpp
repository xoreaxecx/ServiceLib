// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "ServiceControl.h"
#include <iostream>
#include <Windows.h>
#include <winsvc.h>
#include <atlstr.h>
//#include <psapi.h>

#define EXPORT extern "C" __declspec(dllexport)

typedef intptr_t ItemListHandle;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

//EXPORT bool service_running(std::wstring token)
//{
//	SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
//	if (scm == NULL)
//		return false;
//	LPCWSTR   lpServiceName = (LPCWSTR)token.data();
//
//	SC_HANDLE hService = OpenService(scm, lpServiceName, GENERIC_READ);
//	if (hService == NULL)
//	{
//		CloseServiceHandle(scm);
//		return false;
//	}
//
//	SERVICE_STATUS status;
//	LPSERVICE_STATUS pstatus = &status;
//	if (QueryServiceStatus(hService, pstatus) == 0)
//	{
//		CloseServiceHandle(hService);
//		CloseServiceHandle(scm);
//		return false;
//	}
//
//	CloseServiceHandle(hService);
//	CloseServiceHandle(scm);
//	return (status.dwCurrentState == SERVICE_RUNNING) ? (true) : (false);
//}

std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

//EXPORT bool FooCaller(const char* p)
//{
//	std::wstring str = s2ws(p);
//	return service_running(str);
//}

EXPORT bool CallStartService(const char* name, ServiceEntry** entry)//ItemListHandle* hItems, 
{
	bool srvStarted = false;
	bool statusReceived = false;
	std::wstring wname = s2ws(name);
	LPCWSTR   lpServiceName = (LPCWSTR)wname.data();

	SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_ALL);
	if (scm == NULL)
		return false;

	SC_HANDLE hService = OpenService(scm, lpServiceName, GENERIC_ALL);
	if (hService == NULL)
	{
		CloseServiceHandle(scm);
		return false;
	}

	srvStarted = ServiceControl::Start(hService);
	if (srvStarted)
	{
		//auto bytesNeeded = DWORD{ 0 };
		//auto sspInfo = SERVICE_STATUS_PROCESS{ 0 };

		//result = ::QueryServiceStatusEx(
		//	hService,
		//	SC_STATUS_PROCESS_INFO,
		//	reinterpret_cast<LPBYTE>(&sspInfo),
		//	sizeof(sspInfo),
		//	&bytesNeeded);

		//if (result != 0)
		//{
		//	entry.PID = ServiceControl::WStr2BSTR(std::to_wstring(sspInfo.dwProcessId));
		//	entry.StatusString = ServiceControl::WStr2BSTR(ServiceControl::ServiceStatusToString(
		//		static_cast<ServiceStatus>(sspInfo.dwCurrentState)));
		//}

		ServiceEntry* temp = new ServiceEntry();
		statusReceived = ServiceControl::GetStatus(hService, temp);
		if (statusReceived)
		{
			//*hItems = reinterpret_cast<ItemListHandle>(temp);
			*entry = temp;
		}
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(scm);
	return srvStarted && statusReceived;
}

EXPORT bool CallStopService(const char* name, ServiceEntry** entry)
{
	bool srvStopped = false;
	bool statusReceived = false;
	std::wstring wname = s2ws(name);
	LPCWSTR   lpServiceName = (LPCWSTR)wname.data();

	SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_ALL);
	if (scm == NULL)
		return false;

	SC_HANDLE hService = OpenService(scm, lpServiceName, GENERIC_ALL);
	if (hService == NULL)
	{
		CloseServiceHandle(scm);
		return false;
	}

	srvStopped = ServiceControl::Stop(hService, scm);
	if (srvStopped)
	{
		ServiceEntry* temp = new ServiceEntry();
		statusReceived = ServiceControl::GetStatus(hService, temp);
		if (statusReceived)
		{
			*entry = temp;
		}
	}
}

EXPORT bool CallRestartService(const char* name, ServiceEntry** entry) 
{
	bool srvRestarted = false;
	bool statusReceived = false;
	std::wstring wname = s2ws(name);
	LPCWSTR   lpServiceName = (LPCWSTR)wname.data();

	SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_ALL);
	if (scm == NULL)
		return false;

	SC_HANDLE hService = OpenService(scm, lpServiceName, GENERIC_ALL);
	if (hService == NULL)
	{
		CloseServiceHandle(scm);
		return false;
	}

	srvRestarted = ServiceControl::Restart(hService, scm);
	if (srvRestarted)
	{
		ServiceEntry* temp = new ServiceEntry();
		statusReceived = ServiceControl::GetStatus(hService, temp);
		if (statusReceived)
		{
			*entry = temp;
		}
	}
}

EXPORT bool GenerateItems(ItemListHandle* hItems, ServiceEntry** itemsFound, int* itemCount)
{
	//auto services = ServiceControl::EnumServices();
	//std::vector<ServiceEntry> *vec = &services;

	//std::vector<TestS>* vec = new std::vector<TestS>();

	//for (int i = 0; i < 200; i++)
	//{
	//	TestS tempS;
	//	//tempS.fst = i;
	//	tempS.snd = _bstr_t(std::to_wstring(i).c_str()).copy();
	//	//tempS.snd = SysAllocString(std::to_wstring(i).c_str());
	//	vec->push_back(tempS);
	//}

	//std::vector<ServiceEntry>* vec = ServiceControl::EnumServices();
	//std::vector<ServiceEntry> serv = ServiceControl::EnumServices();
	//*vec = serv;

	std::vector<ServiceEntry>* vec = new std::vector<ServiceEntry>();
	*vec = ServiceControl::EnumServices();

	*hItems = reinterpret_cast<ItemListHandle>(vec);
	*itemsFound = vec->data();
	*itemCount = vec->size();

	return true;
}

EXPORT bool ReleaseItems(ItemListHandle hItems)
{
	auto items = reinterpret_cast<std::vector<ServiceEntry>*>(hItems);
	delete items;

	return true;
}

EXPORT bool ReleaseEntry(ItemListHandle hEntry)
{
	auto entry = reinterpret_cast<ServiceEntry**>(hEntry);
	delete entry;

	return true;
}