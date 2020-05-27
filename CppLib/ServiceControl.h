#pragma once

#include <windows.h>
#include <Winsvc.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <wtypes.h>
#include <oleauto.h>
#include <comdef.h>
#include <comutil.h>

using namespace std::literals::chrono_literals;


#ifdef UNICODE
#define ServiceString   std::wstring
#else
#define ServiceString   std::string
#endif

struct TestS
{
    //int fst;
    BSTR snd;
    //_bstr_t snd;
    //CComBSTR snd;

    //~TestS()
    //{
    //    ::SysFreeString(snd);
    //}
};


struct ServiceEntry
{
    BSTR           Name;
    BSTR           Description;
    BSTR           StatusString;
    BSTR           Group;
    BSTR           Path;
    BSTR           PID;
};

enum class ServiceStatus
{
    Unknown = 0,
    Stopped = SERVICE_STOPPED,
    Starting = SERVICE_START_PENDING,
    Stopping = SERVICE_STOP_PENDING,
    Running = SERVICE_RUNNING,
    Continuing = SERVICE_CONTINUE_PENDING,
    Pausing = SERVICE_PAUSE_PENDING,
    Paused = SERVICE_PAUSED
};

class ServiceControl
{
public:
    static ServiceString ServiceStatusToString(ServiceStatus const status)
    {
        switch (status)
        {
        case ServiceStatus::Stopped:     return _T("Stopped");
        case ServiceStatus::Starting:    return _T("Starting");
        case ServiceStatus::Stopping:    return _T("Stopping");
        case ServiceStatus::Running:     return _T("Running");
        case ServiceStatus::Continuing:  return _T("Continuing");
        case ServiceStatus::Pausing:     return _T("Pausing");
        case ServiceStatus::Paused:      return _T("Paused");
        default:                         return _T("Unknown");
        }
    }

    static BSTR WStr2BSTR(std::wstring wstr)
    {
        auto temp = wstr.c_str();
        return _bstr_t(temp).copy();
    }

    static bool GetStatus(SC_HANDLE srvHandle, ServiceEntry* entry)
    {
        //ServiceEntry entry;
        bool result = false;

        if (srvHandle != 0)
        {
            auto bytesNeeded = DWORD{ 0 };
            auto ssp = SERVICE_STATUS_PROCESS{ 0 };

            result = ::QueryServiceStatusEx(
                srvHandle,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&ssp),
                sizeof(ssp),
                &bytesNeeded);

            if (result != 0)
            {
                entry->PID = WStr2BSTR(std::to_wstring(ssp.dwProcessId));
                entry->StatusString = WStr2BSTR(ServiceStatusToString(
                    static_cast<ServiceStatus>(ssp.dwCurrentState)));

                //------
                entry->Description = WStr2BSTR(L"qwe");
                entry->Group = WStr2BSTR(L"qwe");
                entry->Name = WStr2BSTR(L"qwe");
                entry->Path = WStr2BSTR(L"qwe");
                //------
            }
        }

        return result;
    }

    static bool Start(SC_HANDLE srvHandle)
    {
        auto success = false;

        if (srvHandle)
        {
            success = ::StartService(srvHandle, 0, nullptr);
        }

        return success;
    }

    static bool ChangeServiceStatus
    (SC_HANDLE const handle, DWORD const controlCode, SERVICE_STATUS_PROCESS& ssp)
    {
        auto success = false;

        if (handle)
        {
            auto result = ::ControlService(
                handle,
                controlCode,
                reinterpret_cast<LPSERVICE_STATUS>(&ssp));

            success = result != 0;
        }

        return success;
    }

    static bool WaitForStatus(SC_HANDLE srvHandle, ServiceStatus desiredStatus,
        std::chrono::milliseconds const timeout = 30000ms)
    {
        auto success = false;

        if (srvHandle)
        {
            auto ssp = SERVICE_STATUS_PROCESS{ 0 };

            auto bytesNeeded = DWORD{ 0 };

            if (::QueryServiceStatusEx(
                srvHandle,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&ssp),
                sizeof(ssp),
                &bytesNeeded))
            {
                success = WaitForStatus(srvHandle, ssp, desiredStatus, timeout);
            }
        }

        return success;
    }

    static std::chrono::milliseconds GetWaitTime(DWORD const waitHint)
    {
        auto waitTime = waitHint / 10;

        if (waitTime < 1000)
            waitTime = 1000;
        else if (waitTime > 10000)
            waitTime = 10000;

        return std::chrono::milliseconds(waitTime);
    }

    static bool WaitForStatus(SC_HANDLE const handle,
        SERVICE_STATUS_PROCESS& ssp,
        ServiceStatus const desireStatus,
        std::chrono::milliseconds const timeout = 30000ms)
    {
        auto success = ssp.dwCurrentState == static_cast<DWORD>(desireStatus);

        if (!success && handle)
        {
            auto start = std::chrono::high_resolution_clock::now();
            auto waitTime = GetWaitTime(ssp.dwWaitHint);

            while (ssp.dwCurrentState != static_cast<DWORD>(desireStatus))
            {
                std::this_thread::sleep_for(waitTime);

                auto bytesNeeded = DWORD{ 0 };

                if (!::QueryServiceStatusEx(
                    handle,
                    SC_STATUS_PROCESS_INFO,
                    reinterpret_cast<LPBYTE>(&ssp),
                    sizeof(ssp),
                    &bytesNeeded))
                    break;

                if (ssp.dwCurrentState == static_cast<DWORD>(desireStatus))
                {
                    success = true;
                    break;
                }

                if (std::chrono::high_resolution_clock::now() - start > timeout)
                    break;
            }
        }

        return success;
    }

    static bool StopDependentServices(SC_HANDLE srvHandle, SC_HANDLE scm)
    {
        auto ess = ENUM_SERVICE_STATUS{ 0 };
        auto bytesNeeded = DWORD{ 0 };
        auto count = DWORD{ 0 };

        if (!::EnumDependentServices(
            srvHandle,
            SERVICE_ACTIVE,
            nullptr,
            0,
            &bytesNeeded,
            &count))
        {
            if (GetLastError() != ERROR_MORE_DATA)
                return false;

            std::vector<unsigned char> buffer(bytesNeeded, 0);

            if (!::EnumDependentServices(
                srvHandle,
                SERVICE_ACTIVE,
                reinterpret_cast<LPENUM_SERVICE_STATUS>(buffer.data()),
                bytesNeeded,
                &bytesNeeded,
                &count))
            {
                return false;
            }

            for (auto i = DWORD{ 0 }; i < count; ++i)
            {
                auto ess = static_cast<ENUM_SERVICE_STATUS>
                    (*(reinterpret_cast<LPENUM_SERVICE_STATUS>(buffer.data() + i)));

                SC_HANDLE handle = ::OpenService(
                    scm,
                    ess.lpServiceName,
                    SERVICE_STOP | SERVICE_QUERY_STATUS);

                if (!handle)
                    return false;

                auto ssp = SERVICE_STATUS_PROCESS{ 0 };

                if (!ChangeServiceStatus(handle, SERVICE_CONTROL_STOP, ssp))
                    return false;

                if (!WaitForStatus(handle, ssp, ServiceStatus::Stopped))
                    return false;
            }
        }

        return true;
    }

    static bool Stop(SC_HANDLE srvHandle, SC_HANDLE scm)
    {
        auto success = false;

        if (srvHandle && scm)
        {
            success = StopDependentServices(srvHandle, scm);

            if (success)
            {
                auto ssp = SERVICE_STATUS_PROCESS{ 0 };
                success = ChangeServiceStatus(srvHandle, SERVICE_CONTROL_STOP, ssp);
            }
        }

        return success;
    }

    static bool Restart(SC_HANDLE srvHandle, SC_HANDLE scm)
    {
        auto success = false;

        if (srvHandle && scm)
        {
            success = Stop(srvHandle, scm);

            if (success)
            {
                success = Start(srvHandle);
            }
        }

        return success;
    }

    static std::vector<ServiceEntry> EnumServices(
        DWORD const type = SERVICE_WIN32,
        DWORD const state = SERVICE_STATE_ALL,
        ServiceString const* machine = nullptr,
        ServiceString const* dbname = nullptr,
        ServiceString const* groupName = nullptr)
    {
        std::vector<ServiceEntry> ssps;

        SC_HANDLE scm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ENUMERATE_SERVICE);

        auto bytesNeeded = DWORD{ 0 };
        auto servicesReturnedCount = DWORD{ 0 };
        auto resumeHandle = DWORD{ 0 };

        do
        {
            if (!EnumServicesStatusEx(
                scm,
                SC_ENUM_PROCESS_INFO,
                static_cast<DWORD>(type),
                static_cast<DWORD>(state),
                nullptr,
                0,
                &bytesNeeded,
                &servicesReturnedCount,
                &resumeHandle,
                groupName == nullptr ? nullptr : groupName->c_str()))
            {
                DWORD err = GetLastError();
                if (ERROR_MORE_DATA == ::GetLastError())
                {
                    std::vector<unsigned char> buffer(bytesNeeded, 0);

                    if (EnumServicesStatusEx(
                        scm,
                        SC_ENUM_PROCESS_INFO,
                        static_cast<DWORD>(type),
                        static_cast<DWORD>(state),
                        reinterpret_cast<LPBYTE>(buffer.data()),
                        bytesNeeded,
                        &bytesNeeded,
                        &servicesReturnedCount,
                        nullptr,
                        groupName == nullptr ? nullptr : groupName->c_str()))
                    {
                        auto essp = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(buffer.data());

                        for (auto i = DWORD{ 0 }; i < servicesReturnedCount; ++i)
                        {
                            auto ssp = ServiceEntry{};
                            auto srvHandle = OpenService(scm, essp[i].lpServiceName, GENERIC_READ);

                            if (srvHandle != 0)
                            {
                                auto bytesNeeded = DWORD{ 0 };

                                if (!QueryServiceConfig(
                                    srvHandle,
                                    nullptr,
                                    0,
                                    &bytesNeeded))
                                {
                                    if (ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
                                    {
                                        std::vector<unsigned char> buffer(bytesNeeded, 0);

                                        auto lpsc = reinterpret_cast<LPQUERY_SERVICE_CONFIG>(buffer.data());
                                        if (QueryServiceConfig(
                                            srvHandle,
                                            lpsc,
                                            bytesNeeded,
                                            &bytesNeeded))
                                        {
                                            ssp.Path = WStr2BSTR(lpsc->lpBinaryPathName);
                                            ssp.Group = WStr2BSTR(lpsc->lpLoadOrderGroup);
                                        }
                                    }
                                }

                                bytesNeeded = DWORD{ 0 };
                                auto sspInfo = SERVICE_STATUS_PROCESS{ 0 };

                                auto result = ::QueryServiceStatusEx(
                                    srvHandle,
                                    SC_STATUS_PROCESS_INFO,
                                    reinterpret_cast<LPBYTE>(&sspInfo),
                                    sizeof(sspInfo),
                                    &bytesNeeded);

                                if (result != 0)
                                {
                                    DWORD temp = sspInfo.dwProcessId;
                                    if (temp > 0)
                                    {
                                        ssp.PID = _bstr_t(std::to_wstring(temp).c_str()).copy();
                                    }
                                    else
                                    {
                                        ssp.PID = BSTR();
                                    }
                                    //ssp.PID = sspInfo.dwProcessId;
                                }

                                CloseServiceHandle(srvHandle);
                            }

                            ssp.Name = WStr2BSTR(essp[i].lpServiceName);
                            ssp.Description = WStr2BSTR(essp[i].lpDisplayName);
                            ssp.StatusString = WStr2BSTR(ServiceStatusToString(
                                static_cast<ServiceStatus>(essp[i].ServiceStatusProcess.dwCurrentState)));

                            ssps.push_back(ssp);
                        }
                    }
                    else break;
                }
                else break;
            }
        } while (resumeHandle != 0);

        if (scm != 0)
            CloseServiceHandle(scm);
        return ssps;
    }
};