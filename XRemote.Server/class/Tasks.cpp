#include "stdafx.h"
#include "hxc.h"

hxc::Task::Task(const FUNCTION & function, DWORD_PTR Params, ULONG Flags/* = WT_EXECUTEDEFAULT*/) :
    m_pTaskContext(new TASK_CONTEXT(function, Params, Flags))
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
        m_pTaskContext = t.m_pTaskContext;
        m_pTaskContext->AddRef();
    }
    return *this;
}

hxc::Task::~Task()
{
    m_pTaskContext->Release();
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

hxc::Task hxc::Task::FromAsync(IAsyncResult * asyncResult, const ASYNCCALLBACK& endMethod, ULONG Flags)
{
    ASYNCCALLBACK end(endMethod);
    Task t([end](DWORD_PTR Param)
    {
        IAsyncResult* async = reinterpret_cast<IAsyncResult*>(Param);
        end(async);
        delete async;
        return 0;
    }, reinterpret_cast<DWORD_PTR>(asyncResult), Flags);
    return t;
}

hxc::Task hxc::Task::FromAsync1(IAsyncResult * asyncResult, const std::function<DWORD_PTR(IAsyncResult*)>& endMethod, ULONG Flags)
{
    std::function<DWORD_PTR(IAsyncResult*)> end(endMethod);
    Task t([end](DWORD_PTR Param)
    {
        IAsyncResult* async = reinterpret_cast<IAsyncResult*>(Param);
        DWORD_PTR ret = end(async);
        delete async;
        return ret;
    }, reinterpret_cast<DWORD_PTR>(asyncResult), Flags);
    return t;
}

hxc::Task::Task() : m_pTaskContext(nullptr)
{
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
        ret = pctx->m_Proc(pctx->m_Params);
        pctx->m_Status = TaskStatus::RanToCompletion;
    }
    catch (const Exception& e)
    {
        pctx->m_Status = TaskStatus::Faulted;
        pctx->m_Exception = e;
    }

    ::EnterCriticalSection(&pctx->m_Lock);
    pctx->m_Result = ret;
    if (pctx->m_hEvent != NULL)
        ::SetEvent(pctx->m_hEvent);
    ::LeaveCriticalSection(&pctx->m_Lock);

    if (InterlockedExchangeAdd(&pctx->m_Ref, 0) == 0)
        delete pctx;

    return 0;
}

hxc::Task::_TaskContext::_TaskContext(const FUNCTION& function, DWORD_PTR Params, LONG Flags) :
    m_Proc(function), m_Params(Params), m_hEvent(NULL), m_Result(0), m_CreationFlags(Flags), m_Ref(1),
    m_Status(TaskStatus::Created)
{
    ZeroMemory(&m_Lock, sizeof(CRITICAL_SECTION));
    ::InitializeCriticalSection(&m_Lock);
}

hxc::Task::_TaskContext::~_TaskContext()
{
    _ASSERT(m_Ref == 0);

    if (m_hEvent != NULL)
        _DataPool::ManualResetEventPool().Push(m_hEvent);
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
        if (m_hEvent == NULL)
            m_hEvent = _DataPool::ManualResetEventPool().Pop();
        HANDLE hEvent = m_hEvent;
        ::LeaveCriticalSection(&m_Lock);
        ::WaitForSingleObject(hEvent, millisecondsTimeout);
    }
    else
    {
        throw InvalidOperationException();
    }

    if (m_Status == Faulted)
        throw m_Exception;
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
