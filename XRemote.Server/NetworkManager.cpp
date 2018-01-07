#include "stdafx.h"
#include "hxc.h"
#include "NetworkManager.h"

hxc::TcpClient NetworkManager::_Connection;
hxc::CStub<ClassFactoryStub>* NetworkManager::_ClassFactoryStub;
hxc::Task* NetworkManager::_EstablishConnectionTask;
HANDLE NetworkManager::_EventStop;

DWORD_PTR NetworkManager::EstablishConnection(DWORD_PTR Param)
{
    while (::WaitForSingleObject(_EventStop, 0) == WAIT_TIMEOUT)
    {
        try
        {
            _Connection.ConnectAsync(L"127.0.0.1", 27015).Wait(INFINITE);
        }
        catch (const hxc::Exception&)
        {
            ::Sleep(10000);
            continue;
        }

        hxc::CStub<ClassFactoryStub>::CreateInstance(_Connection, reinterpret_cast<void**>(&_ClassFactoryStub));

        WSAEVENT hClose = ::WSACreateEvent();
        ::WSAEventSelect((SOCKET)_Connection.get__Client(), hClose, FD_CLOSE);
        HANDLE h[2] = { _EventStop, hClose };
        DWORD wait = ::WSAWaitForMultipleEvents(2, h, FALSE, WSA_INFINITE, FALSE);
        if (wait == WSA_WAIT_EVENT_0)
        {
            ::WSACloseEvent(hClose);
            break;
        }

        WSANETWORKEVENTS events = {};
        ::WSAEnumNetworkEvents((SOCKET)_Connection.get__Client(), hClose, &events);
        int err = events.iErrorCode[FD_CLOSE_BIT];
        ::WSACloseEvent(hClose);

        hxc::IAsyncResult* async = _Connection.get__Client().BeginDisconnect(nullptr, NULL);
        _Connection.get__Client().EndDisconnect(async);
        delete async;
    }

    return 0;
}

void NetworkManager::Start()
{
    _EventStop = hxc::_DataPool::ManualResetEventPool().Pop();

    _EstablishConnectionTask = new hxc::Task(&EstablishConnection, NULL, WT_EXECUTEINLONGTHREAD);

    _EstablishConnectionTask->Wait(INFINITE);
}
