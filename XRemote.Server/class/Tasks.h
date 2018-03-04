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
        Canceled,
        Faulted
    };

    class Task
    {
    public:
        typedef std::function<DWORD_PTR(DWORD_PTR, HANDLE)> FUNCTION;

        Task(const FUNCTION& function, DWORD_PTR Params, ULONG Flags = WT_EXECUTEDEFAULT);
        Task(const Task& t);
        Task& operator=(const Task& t);
        ~Task();
    private:
        Task();
        Task(const FUNCTION& function, DWORD_PTR Params, ULONG Flags, bool IsOverlappedTask);
    public:
        DWORD_PTR get__Result();
        static Task get__CompletedTask();
    public:
        void Start();
        void Wait(DWORD millisecondsTimeout = INFINITE);
        void Cancel();
        Task ContinueWith(std::function<DWORD_PTR(Task)> delegate, ULONG Flags = WT_EXECUTEDEFAULT);
        static void WaitAll(const std::vector<Task>& tasks, int millisecondsTimeout);
        static int WaitAny(const std::vector<Task>& tasks, int millisecondsTimeout);
        static Task FromAsync(
            std::shared_ptr<IAsyncResult> asyncResult,
            const ASYNCCALLBACK& endMethod,
            ULONG Flags = WT_EXECUTEDEFAULT
        );
        static Task FromAsync1(
            std::shared_ptr<IAsyncResult> asyncResult,
            const std::function<DWORD_PTR(std::shared_ptr<IAsyncResult>)>& endMethod,
            ULONG Flags = WT_EXECUTEDEFAULT
        );

        template<class _Fx, class... _Typ>
        static FUNCTION BindFunction(_Fx&& fx, _Typ&&... t)
        {
            using namespace std;
            return bind(fx, (t)..., placeholders::_1, placeholders::_2);
        }

    private:
        std::vector<DWORD_PTR>& get_InternalParams();

        typedef class _TaskContext
        {
        private:
            static DWORD WINAPI ThreadPoolCallback(PVOID Param);
        public:
            _TaskContext(const FUNCTION& function, DWORD_PTR Params, LONG Flags, bool IsOverlappedTask);
            _TaskContext(const _TaskContext&) = delete;
            _TaskContext& operator=(const _TaskContext&) = delete;
            _TaskContext();
            ~_TaskContext();

            void Start();
            void Wait(DWORD millisecondsTimeout);
            void Cancel(bool WaitEnd = true);
            Task ContinueWith(std::function<DWORD_PTR(Task)> delegate, ULONG Flags);

            DWORD_PTR get__Result();
            std::vector<DWORD_PTR>& get_InternalParams();

            void AddRef(void);
            void Release(void);
        private:
            HANDLE m_hEventWait;
            HANDLE m_hEventCancel;
            FUNCTION m_Proc;
            DWORD_PTR m_Params;
            std::vector<DWORD_PTR> m_InternalParams;
            volatile DWORD_PTR m_Result;
            volatile TaskStatus m_Status;
            CRITICAL_SECTION m_Lock;
            LONG m_CreationFlags;
            volatile LONG m_Ref;
            std::exception_ptr m_Exception;
            bool m_IsOverlappedTask;
            std::shared_ptr<AsyncResultImpl> m_AsyncContext;
        } TASK_CONTEXT;

        TASK_CONTEXT* m_pTaskContext;
        explicit Task(TASK_CONTEXT* p);
    };
}

#endif// if !defined _HXC_TASK_H_
