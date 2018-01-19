#pragma once

#ifndef _SERVICE_H_
#define _SERVICE_H_

#ifdef _SVC_DLL

__declspec(dllexport) void WINAPI ServiceMainW(DWORD dwArgc, LPWSTR* lpszArgv);
__declspec(dllexport) void WINAPI InstallServiceW(HWND, HINSTANCE, LPWSTR, int);
__declspec(dllexport) void WINAPI UninstallServiceW(HWND, HINSTANCE, LPWSTR, int);

#endif // _SVC_DLL

namespace hxc
{
    class ServiceBase;

#ifdef _SVC_EXE
    typedef struct _tagServiceMapEntry
    {
        LPWSTR lpServiceName;
        ServiceBase*(__stdcall *pfnCreateNew)();
        ServiceBase* pInstance;
        StdCallThunk* pServiceMainTrunk;
    } SERVICE_MAP_ENTRYW, *PSERVICE_MAP_ENTRYW;

    class ServiceHost
    {
    public:
        ServiceHost();
        ~ServiceHost();
        LPSERVICE_TABLE_ENTRYW get__DispatchTable();
    private:
        PSERVICE_MAP_ENTRYW m_pMap;
        LPSERVICE_TABLE_ENTRYW _DispatchTable;
    };
#endif // SVC_EXE

#define DECLARE_SERVICE_NAME(name) \
    public: inline virtual const std::wstring ServiceName(){return name;}

	class ServiceBase
	{
    public:
        ServiceBase();
        ~ServiceBase();
        inline void CanHandlePowerEvent(BOOL bCanHandlePowerEvent);
        inline void CanHandleSessionChangeEvent(BOOL bCanHandleSessionChangeEvent);
        inline void CanPauseAndContinue(BOOL bCanPauseAndContinue);
        inline void CanShutdown(BOOL bCanShutdown);
        inline void CanStop(BOOL bCanStop);
        inline void ExitCode(int ExitCode);
        inline HMODULE ModuleHandle();
        void ReportEvent();
        void SetServiceStatus(DWORD dwCurrState, DWORD dwWin32Error = NO_ERROR, DWORD dwCustomError = 0) throw();
        virtual const std::wstring ServiceName() = 0;
        void SetStatusCheckPoint(DWORD dwCheckPoint);
        void AddStatusCheckPoint();
        void SetStatusWaitHint(DWORD dwWaitHint);
        inline SERVICE_STATUS_HANDLE StatusHandle();
        virtual void WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv);
        virtual void OnContinue();
        virtual void OnPause();
        virtual void OnPreShutdown();
        virtual void OnShutdown();
        virtual void OnStart();
        virtual void OnStop();
        virtual void InstallService();
        virtual void UninstallService();
    protected:
        DWORD WINAPI ServiceHandlerEx(DWORD dwControl, DWORD, LPVOID, LPVOID lpContext);
    protected:
        SERVICE_STATUS_HANDLE m_hStatusHandle;
        SERVICE_STATUS m_Status;
        int m_ExitCode;
    private:
        StdCallThunk m_ServiceHandlerExTrunk;
        HANDLE m_hEventExit;
        //HANDLE m_hWaitHandle;
        HMODULE m_hModule;
	};
};//namespace hxc

#endif//if !defined _SERVINSTALLER_H_