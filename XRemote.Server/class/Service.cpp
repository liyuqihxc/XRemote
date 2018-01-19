#include "stdafx.h"
#include "hxc.h"
#include <crtdbg.h>
#include <process.h>

//sc create xremote binPath= "E:\Administrator\Documents\Visual Studio 2015\Projects\WorkingSet\Debug\XRemote.Server.exe" start= demand

#ifdef _SVC_DLL
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            g_pServiceApp->m_hModule = hModule;
            ::DisableThreadLibraryCalls(hModule);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#elif defined _SVC_EXE
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    extern hxc::ServiceHost __host_;

    // This call returns when the service has stopped.
    // The process should simply terminate when the call returns.
    if (!StartServiceCtrlDispatcherW(__host_.get__DispatchTable()))
        return GetLastError();
    return 0;
}
#endif//if defined _DLL


namespace hxc
{
    ServiceHost::~ServiceHost()
    {
        if (_DispatchTable != nullptr)
            delete _DispatchTable;
        for (int i = 0; m_pMap[i].pfnCreateNew != nullptr; i++)
        {
            if (m_pMap[i].pInstance != nullptr)
                delete m_pMap[i].pInstance;
            if (m_pMap[i].pServiceMainTrunk != nullptr)
                delete m_pMap[i].pServiceMainTrunk;
        }
    }

    LPSERVICE_TABLE_ENTRYW ServiceHost::get__DispatchTable()
    {
        typedef void(__stdcall *SERVICE_MAIN)(DWORD, LPWSTR*);

        if (_DispatchTable == nullptr)
        {
            std::vector<SERVICE_TABLE_ENTRYW> list;
            for (int i = 0; m_pMap[i].pfnCreateNew != nullptr; i++)
            {
                ServiceBase*pInst = m_pMap[i].pfnCreateNew(); // 创建服务程序实例
                m_pMap[i].pInstance = pInst;
                m_pMap[i].pServiceMainTrunk = new StdCallThunk((void*)pInst, &ServiceBase::ServiceMain); // 服务程序ServiceMain函数Trunk
                SERVICE_MAIN sm = m_pMap[i].pServiceMainTrunk->get__FunctionPtr<SERVICE_MAIN>();
                SERVICE_TABLE_ENTRYW entry = { m_pMap[i].lpServiceName, sm };
                list.push_back(entry);
            }
            _DispatchTable = new SERVICE_TABLE_ENTRYW[list.size()];
            for (unsigned int i = 0; i < list.size(); i++)
            {
                _DispatchTable[i] = list[i];
            }
        }
        return _DispatchTable;
    }

    ServiceBase::ServiceBase() :
        m_hStatusHandle(NULL), m_ExitCode(0),
        m_hEventExit(NULL), m_ServiceHandlerExTrunk(this, &ServiceBase::ServiceHandlerEx),
        m_hModule(NULL)
    {
        m_Status.dwCurrentState = SERVICE_STOPPED;
        m_Status.dwCheckPoint = 0;
#ifdef _USE_DLL
        m_Status.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
#else
        m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
#endif
        m_Status.dwWin32ExitCode = NO_ERROR;
        m_Status.dwServiceSpecificExitCode = 0;
        m_Status.dwWaitHint = 3000;
        m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN |
            SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SESSIONCHANGE;
    }

    ServiceBase::~ServiceBase() {}

    void WINAPI ServiceBase::ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
    {
        typedef DWORD (WINAPI *HandlerEx)(DWORD, DWORD, LPVOID, LPVOID);
        HandlerEx h = m_ServiceHandlerExTrunk.get__FunctionPtr<HandlerEx>();

        SERVICE_STATUS_HANDLE hStatus = ::RegisterServiceCtrlHandlerExW(ServiceName().c_str(), h, nullptr);
        if (hStatus == NULL)
        {
            //Report Error
            return;
        }
        m_hStatusHandle = hStatus;

        SetServiceStatus(SERVICE_START_PENDING);

        m_hEventExit = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (m_hEventExit == NULL)
        {
            SetServiceStatus(SERVICE_STOPPED, ::GetLastError());
            _ASSERT(FALSE);
            //Report Error
            return;
        }

        try
        {
            OnStart();
            SetServiceStatus(SERVICE_RUNNING);
        }
        catch (const Exception& e)
        {
            SetServiceStatus(SERVICE_STOPPED, e.get__HResult());
            _ASSERT(FALSE);
            //Report Error
        }

        if (WaitForSingleObject(m_hEventExit, INFINITE) == WAIT_OBJECT_0)
        {
            try
            {
                OnStop();
                SetServiceStatus(SERVICE_STOPPED);
                CloseHandle(m_hEventExit);
            }
            catch (const Exception& e)
            {
                SetServiceStatus(SERVICE_STOPPED, e.get__HResult());
                //Report Error
            }
        }
    }

    void ServiceBase::OnContinue() {}

    void ServiceBase::OnPause() {}

    void ServiceBase::OnPreShutdown() {}

    void ServiceBase::OnShutdown() {}

    void ServiceBase::OnStart() {}

    void ServiceBase::OnStop() {}

    void ServiceBase::InstallService()
    {
    }

    void ServiceBase::UninstallService()
    {
    }

    DWORD WINAPI ServiceBase::ServiceHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
    {
        switch (dwControl)
        {
        case SERVICE_CONTROL_CONTINUE:
        {
            try
            {
                SetServiceStatus(SERVICE_CONTINUE_PENDING);
                OnContinue();
                SetServiceStatus(SERVICE_RUNNING);
            }
            catch (const Exception&)
            {

            }
        }
        break;
        case SERVICE_CONTROL_INTERROGATE://SCM要求服务报告当前状态
            //The handler should simply return NO_ERROR;
            //the SCM is aware of the current state of the service.
            break;
        case SERVICE_CONTROL_PAUSE:
        {
            try
            {

            }
            catch (const Exception&)
            {

            }
        }
        break;
        case SERVICE_CONTROL_PRESHUTDOWN:
        {
            try
            {

            }
            catch (const Exception&)
            {

            }
        }
        break;
        case SERVICE_CONTROL_SHUTDOWN:
        {
            try
            {

            }
            catch (const Exception&)
            {

            }
        }
        break;
        case SERVICE_CONTROL_STOP:
        {
            try
            {
                SetServiceStatus(SERVICE_STOP_PENDING);
                ::SetEvent(this->m_hEventExit);
            }
            catch (const Exception&)
            {

            }
        }
        break;
        }
        return NO_ERROR;
    }

    void ServiceBase::SetServiceStatus(DWORD dwCurrState, DWORD dwWin32Error /*= NO_ERROR*/, DWORD dwCustomError /*= 0*/) throw()
    {
        SERVICE_STATUS s;
        memcpy(&s, &this->m_Status, sizeof(SERVICE_STATUS));
        s.dwCurrentState = dwCurrState;
        s.dwWin32ExitCode = dwWin32Error;
        s.dwServiceSpecificExitCode = dwCustomError;
        ::SetServiceStatus(this->m_hStatusHandle, &s);
    }

    void ServiceBase::SetStatusCheckPoint(DWORD dwCheckPoint)
    {
        SERVICE_STATUS s;
        memcpy(&s, &this->m_Status, sizeof(SERVICE_STATUS));
        s.dwCheckPoint = dwCheckPoint;
        ::SetServiceStatus(this->m_hStatusHandle, &s);
    }

    void ServiceBase::AddStatusCheckPoint()
    {
        SERVICE_STATUS s;
        memcpy(&s, &this->m_Status, sizeof(SERVICE_STATUS));
        s.dwCheckPoint++;
        ::SetServiceStatus(this->m_hStatusHandle, &s);
    }

    void ServiceBase::SetStatusWaitHint(DWORD dwWaitHint)
    {
        SERVICE_STATUS s;
        memcpy(&s, &this->m_Status, sizeof(SERVICE_STATUS));
        s.dwWaitHint = dwWaitHint;
        ::SetServiceStatus(this->m_hStatusHandle, &s);
    }

    inline SERVICE_STATUS_HANDLE ServiceBase::StatusHandle()
    {
        return this->m_hStatusHandle;
    }

}
