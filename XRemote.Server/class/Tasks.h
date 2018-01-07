#pragma once

#if !defined _HXC_TASK_H_
#define _HXC_TASK_H_

namespace hxc
{
    enum TaskStatus
    {
        Created,
        WaitingToRun,
        Running,
        RanToCompletion,
        //Canceled,
        Faulted
    };

    class Task
    {
    public:
        typedef std::function<DWORD_PTR(DWORD_PTR)> FUNCTION;

        Task(const FUNCTION& function, DWORD_PTR Params, ULONG Flags = WT_EXECUTEDEFAULT);
        Task(const Task& t);
        Task& operator=(const Task& t);
        ~Task();
    public:
        DWORD_PTR get_Result();

    public:
        void Start();
        void Wait(DWORD millisecondsTimeout);
        static void WaitAll(const std::vector<Task>& tasks, int millisecondsTimeout);
        static int WaitAny(const std::vector<Task>& tasks, int millisecondsTimeout);
        static Task FromAsync(
            IAsyncResult* asyncResult,
            const ASYNCCALLBACK& endMethod,
            ULONG Flags = WT_EXECUTEDEFAULT
        );
        static Task FromAsync1(
            IAsyncResult* asyncResult,
            const std::function<DWORD_PTR(IAsyncResult*)>& endMethod,
            ULONG Flags = WT_EXECUTEDEFAULT
        );

    private:
        Task();
        std::vector<DWORD_PTR>& get_InternalParams();

        typedef class _TaskContext
        {
        private:
            _TaskContext(const _TaskContext&) {}
            _TaskContext& operator=(const _TaskContext&) {}
            _TaskContext() {}
            static DWORD WINAPI ThreadPoolCallback(PVOID Param);
        public:
            _TaskContext(const FUNCTION& function, DWORD_PTR Params, LONG Flags);
            ~_TaskContext();

            void Start();
            void Wait(DWORD millisecondsTimeout);

            DWORD_PTR get_Result();
            std::vector<DWORD_PTR>& get_InternalParams();

            void AddRef(void);
            void Release(void);
        private:
            HANDLE m_hEvent;
            FUNCTION m_Proc;
            DWORD_PTR m_Params;
            std::vector<DWORD_PTR> m_InternalParams;
            DWORD_PTR m_Result;
            CRITICAL_SECTION m_Lock;
            TaskStatus m_Status;
            LONG m_CreationFlags;
            volatile LONG m_Ref;
            Exception m_Exception;
        } TASK_CONTEXT;

        TASK_CONTEXT* m_pTaskContext;
    };
}

#endif// if !defined _HXC_TASK_H_
