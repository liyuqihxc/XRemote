#include "stdafx.h"
#include "hxc.h"

hxc::Task::Task(const FUNCTION & function, DWORD_PTR Params, ULONG Flags/* = WT_EXECUTEDEFAULT*/) :
    m_pTaskContext(new TASK_CONTEXT(function, Params, Flags, false))
{
    
}

hxc::Task::Task(const Task & t)
{
    m_pTaskContext = t.m_pTaskContext;
    m_pTaskContext->AddRef();
}

hxc::Task& hxc::Task::operator=(const Task & t)
{
    if (this != &t)
    {
        m_pTaskContext->Release();
        m_pTaskContext = t.m_pTaskContext;
        m_pTaskContext->AddRef();
    }
    return *this;
}

hxc::Task::~Task()
{
    m_pTaskContext->Release();
}

hxc::Task::Task(const FUNCTION & function, DWORD_PTR Params, ULONG Flags, bool NoCancel) :
    m_pTaskContext(new TASK_CONTEXT(function, Params, Flags, NoCancel))
{
}

hxc::Task::Task(hxc::Task::TASK_CONTEXT * p) : m_pTaskContext(p)
{
}

DWORD_PTR hxc::Task::get_Result()
{
    return m_pTaskContext->get_Result();
}

std::vector<DWORD_PTR>& hxc::Task::get_InternalParams()
{
    return m_pTaskContext->get_InternalParams();
}

void hxc::Task::Start()
{
    m_pTaskContext->Start();
}

void hxc::Task::Wait(DWORD millisecondsTimeout)
{
    m_pTaskContext->Wait(millisecondsTimeout);
}

void hxc::Task::Cancel()
{
    m_pTaskContext->Cancel();
}

hxc::Task hxc::Task::ContinueWith(std::function<DWORD_PTR(Task)> delegate, ULONG Flags)
{
    return m_pTaskContext->ContinueWith(delegate, Flags);
}

hxc::Task hxc::Task::FromAsync(std::shared_ptr<IAsyncResult> asyncResult, const ASYNCCALLBACK& endMethod, ULONG Flags)
{
    using namespace std;

    ASYNCCALLBACK end(endMethod);
    Task t([end](DWORD_PTR Param, HANDLE hCancel)
    {
        auto* async = reinterpret_cast<shared_ptr<IAsyncResult>*>(Param);
        end(*async);
        delete async;
        return 0;
    }, reinterpret_cast<DWORD_PTR>(new shared_ptr<IAsyncResult>(asyncResult)), Flags, true);
    t.Start();
    return t;
}

hxc::Task hxc::Task::FromAsync1(std::shared_ptr<IAsyncResult> asyncResult, const std::function<DWORD_PTR(std::shared_ptr<IAsyncResult>)>& endMethod, ULONG Flags)
{
    using namespace std;

    function<DWORD_PTR(shared_ptr<IAsyncResult>)> end(endMethod);
    Task t([end](DWORD_PTR Param, HANDLE hCancel)
    {
        auto* async = reinterpret_cast<shared_ptr<IAsyncResult>*>(Param);
        DWORD_PTR ret = end(*async);
        delete async;
        return ret;
    }, reinterpret_cast<DWORD_PTR>(new shared_ptr<IAsyncResult>(asyncResult)), Flags, true);
    t.Start();
    return t;
}

DWORD WINAPI hxc::Task::_TaskContext::ThreadPoolCallback(PVOID Param)
{
    _TaskContext* pctx = (_TaskContext*)Param;
    ::EnterCriticalSection(&pctx->m_Lock);
    pctx->m_Status = TaskStatus::Running;
    ::LeaveCriticalSection(&pctx->m_Lock);

    DWORD_PTR ret = 0;
    try
    {
        ret = pctx->m_Proc(pctx->m_Params, pctx->m_hEventCancel);
        pctx->m_Status = TaskStatus::RanToCompletion;
    }
    catch (const Exception& e)
    {
        pctx->m_Status = TaskStatus::Faulted;
        pctx->m_Exception = e;
    }

    ::EnterCriticalSection(&pctx->m_Lock);
    pctx->m_Result = ret;
    if (pctx->m_hEventWait != NULL)
        ::SetEvent(pctx->m_hEventWait);
    ::LeaveCriticalSection(&pctx->m_Lock);

    if (InterlockedExchangeAdd(&pctx->m_Ref, 0) == 0)
        delete pctx;

    return 0;
}

hxc::Task::_TaskContext::_TaskContext(const FUNCTION& function, DWORD_PTR Params, LONG Flags, bool IsOverlappedTask) :
    m_Proc(function), m_Params(Params), m_hEventWait(NULL), m_Result(0), m_CreationFlags(Flags), m_Ref(1),
    m_Status(TaskStatus::Created), m_hEventCancel(NULL), m_IsOverlappedTask(IsOverlappedTask)
{
    ZeroMemory(&m_Lock, sizeof(CRITICAL_SECTION));
    ::InitializeCriticalSection(&m_Lock);

    if (!IsOverlappedTask)
    {
        m_hEventCancel = _DataPool::ManualResetEventPool().Pop();
    }
    else
    {
        m_AsyncContext = std::static_pointer_cast<AsyncResultImpl>(*reinterpret_cast<std::shared_ptr<IAsyncResult>*>(Params));
    }
}

hxc::Task::_TaskContext::~_TaskContext()
{
    _ASSERT(m_Ref == 0);

    if (m_hEventWait != NULL)
        _DataPool::ManualResetEventPool().Push(m_hEventWait);
}

void hxc::Task::_TaskContext::Start()
{
    ::EnterCriticalSection(&m_Lock);
    if (::QueueUserWorkItem(ThreadPoolCallback, this, m_CreationFlags))
        m_Status = TaskStatus::WaitingToRun;
    else
        throw Win32Exception(::GetLastError());
    ::LeaveCriticalSection(&m_Lock);
}

void hxc::Task::_TaskContext::Wait(DWORD millisecondsTimeout)
{
    if (m_Status == TaskStatus::Running || m_Status == TaskStatus::WaitingToRun || m_Status == Created)
    {
        if (m_Status == TaskStatus::Created)
            Start();
        ::EnterCriticalSection(&m_Lock);
        if (m_hEventWait == NULL)
            m_hEventWait = _DataPool::ManualResetEventPool().Pop();
        HANDLE hEvent = m_hEventWait;
        ::LeaveCriticalSection(&m_Lock);
        ::WaitForSingleObject(hEvent, millisecondsTimeout);
    }
    else if (m_Status != RanToCompletion)
    {
        throw InvalidOperationException();
    }

    if (m_Status == Faulted)
        throw m_Exception;
}

void hxc::Task::_TaskContext::Cancel(bool WaitEnd)
{
    if (!m_IsOverlappedTask)
        ::SetEvent(m_hEventCancel);
    else
        m_AsyncContext->CancelIo();

    if (WaitEnd)
        Wait(INFINITE);
}

hxc::Task hxc::Task::_TaskContext::ContinueWith(std::function<DWORD_PTR(Task)> delegate, ULONG Flags)
{
    std::function<DWORD_PTR(Task)> Delegate(delegate);
    Task t([Delegate](DWORD_PTR Param, HANDLE hCancel)
    {
        Task t(reinterpret_cast<TASK_CONTEXT*>(Param));
        t.Wait();
        return Delegate(t);
    }, reinterpret_cast<DWORD_PTR>(this), Flags);
    t.Start();
    return t;
}

DWORD_PTR hxc::Task::_TaskContext::get_Result()
{
    ::EnterCriticalSection(&m_Lock);
    if (m_Status == RanToCompletion)
    {
        ::LeaveCriticalSection(&m_Lock);
        return m_Result;
    }
    else
    {
        ::LeaveCriticalSection(&m_Lock);
        throw InvalidOperationException();
    }
}

std::vector<DWORD_PTR>& hxc::Task::_TaskContext::get_InternalParams()
{
    return m_InternalParams;
}

void hxc::Task::_TaskContext::AddRef(void)
{
    ::InterlockedIncrement(&m_Ref);
}

void hxc::Task::_TaskContext::Release(void)
{
    LONG value = ::InterlockedDecrement(&m_Ref);
    _ASSERT(value >= 0);
    if (value == 0)
        delete this;
}
