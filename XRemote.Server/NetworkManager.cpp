#include "stdafx.h"
#include "hxc.h"
#include "NetworkManager.h"

hxc::TcpClient NetworkManager::_Connection;
std::unique_ptr<hxc::CStub<ClassFactoryImpl>> NetworkManager::_ClassFactoryStub;
std::unique_ptr<hxc::Task> NetworkManager::_EstablishConnectionTask;

DWORD_PTR NetworkManager::EstablishConnection(DWORD_PTR Param, HANDLE hCancel)
{
    while (::WaitForSingleObject(hCancel, 0) == WAIT_TIMEOUT)
    {
        try
        {
            hxc::Task t = _Connection.ConnectAsync(L"127.0.0.1", 27015);
            t.Wait();
        }
        catch (const hxc::Exception& e)
        {
            ::Sleep(1000);
            continue;
        }

        hxc::CStub<ClassFactoryImpl>::CreateInstance(_Connection, reinterpret_cast<void**>(&_ClassFactoryStub));

        SOCKET s = _Connection.get__Client()->get__Handle();
        WSAEVENT hClose = ::WSACreateEvent();
        ::WSAEventSelect(s, hClose, FD_CLOSE);
        HANDLE h[2] = { hCancel, hClose };
        DWORD wait = ::WSAWaitForMultipleEvents(2, h, FALSE, WSA_INFINITE, FALSE);
        if (wait == WSA_WAIT_EVENT_0)// ÍË³ö
        {
            ::WSACloseEvent(hClose);
            break;
        }

        WSANETWORKEVENTS events = {};
        ::WSAEnumNetworkEvents(s, hClose, &events);
        int err = events.iErrorCode[FD_CLOSE_BIT];
        ::WSACloseEvent(hClose);

        auto async = _Connection.get__Client()->BeginDisconnect(nullptr, NULL);
        _Connection.get__Client()->EndDisconnect(async);

        _ClassFactoryStub = nullptr;
    }

    return 0;
}

void NetworkManager::Start()
{
    _EstablishConnectionTask.reset(new hxc::Task(&EstablishConnection, NULL, WT_EXECUTEINLONGTHREAD));

    _EstablishConnectionTask->Wait();
}
