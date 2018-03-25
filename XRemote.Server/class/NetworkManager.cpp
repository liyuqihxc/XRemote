#include "stdafx.h"
#include "hxc.h"
#include "NetworkManager.h"

hxc::TcpClient NetworkManager::_Connection(AF_INET);
std::map<int32_t, std::unique_ptr<hxc::InterfaceStub>> _ObjectMap;
hxc::Task NetworkManager::_EstablishConnectionTask(&NetworkManager::EstablishConnection, NULL, WT_EXECUTEINLONGTHREAD);

DWORD_PTR NetworkManager::EstablishConnection(DWORD_PTR Param, HANDLE hCancel)
{
    while (::WaitForSingleObject(hCancel, 0) == WAIT_TIMEOUT)
    {
        try
        {
            hxc::Task t = _Connection.ConnectAsync(L"127.0.0.1", 27015);
            t.Wait();
        }
        catch (const hxc::SocketException&)
        {
            ::Sleep(1000);
            continue;
        }

        SOCKET s = _Connection.get__Client()->get__Handle();
        WSAEVENT hClose = ::WSACreateEvent();
        ::WSAEventSelect(s, hClose, FD_CLOSE);
        HANDLE h[2] = { hCancel, hClose };
        DWORD wait = ::WSAWaitForMultipleEvents(2, h, FALSE, WSA_INFINITE, FALSE);
        if (wait == WSA_WAIT_EVENT_0)// 退出
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
    }

    return 0;
}

void NetworkManager::Start()
{
    _EstablishConnectionTask.Wait();
}
