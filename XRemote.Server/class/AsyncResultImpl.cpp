#include "stdafx.h"
#include "hxc.h"

hxc::AsyncResultImpl::AsyncResultImpl(DWORD_PTR AsyncState, const ASYNCCALLBACK & callback) :
    _AsyncState(AsyncState), _CompletedSynchronously(false), _IsCompleted(false),
    _Callback(callback), _ErrorCode(0), _AsyncWaitHandle(NULL)
    {
        Internal = 0;
        InternalHigh = 0;
        Offset = 0;
        OffsetHigh = 0;
        hEvent = _DataPool::ManualResetEventPool().Pop();

        ::InitializeCriticalSection(&_Lock);
    }

hxc::AsyncResultImpl::~AsyncResultImpl()
{
    _DataPool::ManualResetEventPool().Push(hEvent);
    if (!_AsyncWaitHandle)
        _DataPool::ManualResetEventPool().Push(_AsyncWaitHandle);

    ::DeleteCriticalSection(&_Lock);
}

void hxc::AsyncResultImpl::Complete(DWORD ErrCode, bool CompletedSynchronously)
{
    _ErrorCode = ErrCode;
    _CompletedSynchronously = CompletedSynchronously;
    _IsCompleted = true;

    ::EnterCriticalSection(&_Lock);
    if (_AsyncWaitHandle)
        ::SetEvent(_AsyncWaitHandle);
    ::LeaveCriticalSection(&_Lock);

    if (_Callback != nullptr)
    {
        try
        {
            _Callback(shared_from_this());
        }
        catch (const Exception& e)
        {

        }
    }
}

void hxc::AsyncResultImpl::WaitForCompletion(void)
{
    if (_EndCalled)
    {
        InvalidOperationException e;
        SET_EXCEPTION(e);
        throw e;
    }

    if (!get__IsCompleted())
    {
        ::EnterCriticalSection(&_Lock);
        if (_AsyncWaitHandle == NULL)
            _AsyncWaitHandle = _DataPool::ManualResetEventPool().Pop();
        ::LeaveCriticalSection(&_Lock);

        ::WaitForSingleObject(_AsyncWaitHandle, INFINITE);
    }

    _EndCalled = true;
}

VOID hxc::AsyncResultImpl::IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    AsyncResultImpl* pAsync = static_cast<AsyncResultImpl*>(lpOverlapped);

    pAsync->Complete(dwErrorCode, false);
}

std::vector<DWORD_PTR>& hxc::AsyncResultImpl::get__InternalParams(void)
{
    return _InternalParams;
}

DWORD_PTR hxc::AsyncResultImpl::get__AsyncState()
{
    return _AsyncState;
}

HANDLE hxc::AsyncResultImpl::get__AsyncWaitHandle()
{
    ::EnterCriticalSection(&_Lock);
    if (_AsyncWaitHandle == NULL)
        _AsyncWaitHandle = _DataPool::ManualResetEventPool().Pop();
    ::LeaveCriticalSection(&_Lock);
    return _AsyncWaitHandle;
}

DWORD hxc::AsyncResultImpl::get__ErrorCode()
{
    return InterlockedExchangeAdd(&_ErrorCode, 0);
}

bool hxc::AsyncResultImpl::get__CompletedSynchronously()
{
    return _CompletedSynchronously;
}

bool hxc::AsyncResultImpl::get__IsCompleted()
{
    return InterlockedExchangeAdd(&_IsCompleted, 0) ? true : false;
}
