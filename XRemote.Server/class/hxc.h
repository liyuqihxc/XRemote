#pragma once

#ifndef _HXC_H_
#define _HXC_H_

#include <list>
#include <vector>
#include <map>
#include <stack>
#include <queue>
#include<strsafe.h>
#include <string>
#include <functional>
#include <combaseapi.h>
#include <Unknwn.h>
#include <WinSock2.h>
#include <mswsock.h>

namespace hxc
{
    class DECLSPEC_NOVTABLE IAsyncResult
    {
    public:
        virtual DWORD_PTR get__AsyncState() = 0;
        virtual HANDLE get__AsyncWaitHandle() = 0;
        virtual DWORD get__ErrorCode() = 0;
        virtual bool get__CompletedSynchronously() = 0;
        virtual bool get__IsCompleted() = 0;
    };

    typedef std::function<void(IAsyncResult*)> ASYNCCALLBACK;

    class AsyncResultImpl :
        public IAsyncResult,
        public OVERLAPPED
    {
    protected:
        DWORD_PTR _AsyncState;
        volatile HANDLE _AsyncWaitHandle;
        volatile DWORD _ErrorCode;
        bool _CompletedSynchronously;
        volatile long _IsCompleted;

        ASYNCCALLBACK _Callback;

    private:
        CRITICAL_SECTION _Lock;

        std::vector<DWORD_PTR> _InternalParams;
        bool _EndCalled;

        AsyncResultImpl();
    public:
        AsyncResultImpl(DWORD_PTR AsyncState, const ASYNCCALLBACK& callback);
        ~AsyncResultImpl();

        virtual void Complete(DWORD ErrCode, bool CompletedSynchronously);
        virtual void WaitForCompletion(void);

        std::vector<DWORD_PTR>& get__InternalParams(void);
    public:
        virtual DWORD_PTR get__AsyncState();
        virtual HANDLE get__AsyncWaitHandle();
        virtual DWORD get__ErrorCode();
        virtual bool get__CompletedSynchronously();
        virtual bool get__IsCompleted();
    };
}

#include "Exceptions.h"
#include "Locks.h"
#include "Event.h"
#include "stdTrunk.hpp"
#include "Environment.h"
#include "Crypt.h"
#include "Pool.h"
#include "Tasks.h"
#include "Network.h"
#include "ProxyStub.h"
#include "Service.h"
#include "SystemInfo.h"
#include "Utility.h"

#endif // ! _HXC_H_
