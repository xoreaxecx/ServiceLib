// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "ServiceControl.h"
#include <iostream>
#include <Windows.h>
#include <winsvc.h>
#include <atlstr.h>

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

std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

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
		ServiceEntry* temp = new ServiceEntry();
		statusReceived = ServiceControl::GetStatus(hService, temp);
		if (statusReceived)
		{
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

	CloseServiceHandle(hService);
	CloseServiceHandle(scm);
	return srvStopped && statusReceived;
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

	CloseServiceHandle(hService);
	CloseServiceHandle(scm);
	return srvRestarted && statusReceived;
}

EXPORT bool GenerateItems(ItemListHandle* hItems, ServiceEntry** itemsFound, int* itemCount)
{
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