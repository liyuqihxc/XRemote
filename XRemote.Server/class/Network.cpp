#include "stdafx.h"
#include "hxc.h"
#include <WS2tcpip.h>

#pragma comment(lib,"Ws2_32.lib")

namespace hxc
{
    class SocketAsyncResultImpl : public AsyncResultImpl
    {
    public:
        enum InternalOp_Type
        {
            Accept,
            Connect,
            DisConnect,
            Receive,
            Send
        };

        SocketAsyncResultImpl(Socket&s, InternalOp_Type InternalOp, DWORD_PTR AsyncState, const ASYNCCALLBACK & callback) :
            AsyncResultImpl(AsyncState, callback), _Socket(s), _InternalOperation(InternalOp)
        {
            
        }
    public:
        virtual void Complete(DWORD ErrCode, bool CompletedSynchronously)
        {
            _ErrorCode = SocketException::NTSTATUS_To_Win32Err((NTSTATUS)ErrCode);

            switch (_InternalOperation)
            {
            case SocketAsyncResultImpl::Accept:
                AcceptInternalOp();
                break;
            case SocketAsyncResultImpl::Connect:
                ConnectInternalOp();
                break;
            case SocketAsyncResultImpl::Receive:
                ReceiveInternalOp();
                break;
            default:
                break;
            }

            AsyncResultImpl::Complete(_ErrorCode, CompletedSynchronously);
        }

        virtual void WaitForCompletion(void)
        {
            AsyncResultImpl::WaitForCompletion();
            if (get__ErrorCode() != ERROR_SUCCESS)
                throw SocketException(static_cast<int>(get__ErrorCode()));
        }

        virtual void CancelIo()
        {
            ::CancelIoEx((HANDLE)_Socket.get__Handle(), this);
        }

        void AcceptInternalOp()
        {
            if (get__ErrorCode() == ERROR_SUCCESS)
            {
                DWORD cbTransfer = 0, dwFlags = 0;
                ::WSAGetOverlappedResult(_Socket.get__Handle(), this, &cbTransfer, TRUE, &dwFlags);

                std::shared_ptr<Socket>* s =  reinterpret_cast<std::shared_ptr<Socket>*>(get__InternalParams()[0]);
                std::shared_ptr<Socket> socket(*s);
                ::setsockopt(socket->get__Handle(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                    reinterpret_cast<char*>(_Socket.get__Handle()), sizeof(SOCKET));

                InterlockedCompareExchange(&socket->_Connected, 1, 0);

                PDWORD_PTR pdwSizeTransfered = &get__InternalParams()[1];

                const DWORD sizeAddr = sizeof(sockaddr_in) + 16;
                LPBYTE pBuffer = reinterpret_cast<LPBYTE>(get__InternalParams()[2]);
                sockaddr_in* RemoteAddr = NULL;
                sockaddr_in* LocalAddr = NULL;
                int remoteLen = sizeof(sockaddr_in), localLen = sizeof(sockaddr_in);
                Socket::pfnGetAcceptExSockaddrs(pBuffer, *pdwSizeTransfered, sizeAddr, sizeAddr,
                    reinterpret_cast<sockaddr**>(&LocalAddr), &localLen,
                    reinterpret_cast<sockaddr**>(&RemoteAddr), &remoteLen
                );

                *pdwSizeTransfered = cbTransfer;
            }
        }

        void ConnectInternalOp()
        {
            if (get__ErrorCode() == ERROR_SUCCESS)
            {
                ::setsockopt(_Socket.get__Handle(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
                InterlockedCompareExchange(&_Socket._Connected, 1, 0);
            }
        }

        void ReceiveInternalOp()
        {
            if (get__ErrorCode() != ERROR_SUCCESS)
            {
                DWORD cbTransfer = 0, dwFlags = 0;
                ::WSAGetOverlappedResult(_Socket.get__Handle(), this, &cbTransfer, TRUE, &dwFlags);

                LPDWORD lpdwFlags = reinterpret_cast<LPDWORD>(get__InternalParams()[0]);
                *lpdwFlags = dwFlags;
            }

            if (get__ErrorCode() == ERROR_NETNAME_DELETED)
            {
            }
        }

        /*void* operator new(size_t size)
        {
            free_elements.Lock();
            if (free_elements->empty())
            {
                free_elements.Release();
                return malloc(size);
            }
            void* p = *(free_elements->end());
            free_elements->pop_back();
            free_elements.Release();
            return p;
        }

        void operator delete(void* p)
        {
            free_elements.Lock();
            if (free_elements->size() > 20)
                free(p);
            else
                free_elements->push_back(p);
            free_elements.Release();
        }*/
    private:
        //static Critical_Section<std::vector<void*>> free_elements;
        Socket& _Socket;
        InternalOp_Type _InternalOperation;
    };

    //Critical_Section<std::vector<void*>>  SocketAsyncResultImpl::free_elements;

Socket::Socket(int af, int type, int protocol) :
    _AddressFamily(af), _SocketType(type), _ProtocolType(protocol), _Connected(false)
{
    if (!pfnAcceptEx)
    {
        WSADATA wd;
        InitializeWinsock(MAKEWORD(2, 2), &wd);
    }

    //s = _DataPool::TCPSocketPool().Pop();
    s = ::WSASocket(_AddressFamily, _SocketType, _ProtocolType, nullptr, 0, WSA_FLAG_OVERLAPPED);
    _ASSERT(s != INVALID_SOCKET);

    BOOL b = ::BindIoCompletionCallback((HANDLE)s, AsyncResultImpl::IOCompletionRoutine, 0);
    if (!b)
    {
        Win32Exception e(Exception::NTSTATUS_To_Win32Err((LONG)b));
        SET_EXCEPTION(e);
        throw e;
    }
}

int Socket::get__AddressFamily()
{
    return _AddressFamily;
}

bool Socket::get__Connected()
{
    return _Connected ? true : false;
}

std::shared_ptr<Socket> Socket::Create(int af, int type, int protocol)
{
    return std::shared_ptr<Socket>(new Socket(af, type, protocol));
}

std::shared_ptr<IAsyncResult> Socket::BeginAccept(std::shared_ptr<Socket> AcceptSocket, DWORD receiveSize, const ASYNCCALLBACK & Callback, DWORD_PTR Context)
{
    using namespace std;
    _ASSERT(Socket::pfnAcceptEx != nullptr);

    const DWORD sizeAddr = sizeof(sockaddr_in) + 16;
    if (receiveSize + sizeAddr * 2 > _DataPool::BufferSize)
    {
        ArgumentOutOfRangeException e;
        SET_EXCEPTION(e);
        throw e;
    }

    auto ai = shared_ptr<SocketAsyncResultImpl>(
        new SocketAsyncResultImpl(*this, SocketAsyncResultImpl::Accept, Context, Callback)
    );
    ai->get__InternalParams().push_back(reinterpret_cast<DWORD_PTR>(new shared_ptr<Socket>(AcceptSocket)));
    ai->get__InternalParams().push_back(receiveSize);
    LPBYTE pBuffer = _DataPool::BufferPool().Pop();
    ai->get__InternalParams().push_back(reinterpret_cast<DWORD_PTR>(pBuffer));

    DWORD dwBytesReceived = 0;
    BOOL ret = Socket::pfnAcceptEx(this->s, AcceptSocket->get__Handle(), pBuffer, receiveSize, sizeAddr, sizeAddr, &dwBytesReceived, ai.get());
    int code = ::WSAGetLastError();
    if (ret)
    {
        ai->Complete(static_cast<DWORD>(code), true);
    }
    else if (!ret && code != WSA_IO_PENDING)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        ai.reset();
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return static_pointer_cast<IAsyncResult>(ai);
}

std::shared_ptr<Socket> Socket::EndAccept(LPBYTE* ppArray, LPDWORD pReceiveSize, std::shared_ptr<IAsyncResult> asyncResult)
{
    using namespace std;

    if (!ppArray || !pReceiveSize)
    {
        NullReferenceException e;
        SET_EXCEPTION(e);
        throw e;
    }

    auto async = static_pointer_cast<SocketAsyncResultImpl>(asyncResult);

    async->WaitForCompletion();
    auto ptr = reinterpret_cast<shared_ptr<Socket>*>(async->get__InternalParams()[0]);
    *pReceiveSize = async->get__InternalParams()[1];
    *ppArray = reinterpret_cast<LPBYTE>(async->get__InternalParams()[2]);
    shared_ptr<Socket> temp(*ptr);
    delete ptr;

    return std::shared_ptr<Socket>(temp);
}

void Socket::Bind(const struct sockaddr* name, int namelen)
{
    if (0 != ::bind(s, name, namelen))
    {
        int code = WSAGetLastError();
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
}

void Socket::CloseSocket(void)
{
    if (s == INVALID_SOCKET)
        return;
    if (get__Connected())
    {
        Shutdown(SD_BOTH);
        CancelIo((HANDLE)s);
        auto async = BeginDisconnect(nullptr, NULL);
        EndDisconnect(async);
        _DataPool::TCPSocketPool().Push(s);
        s = INVALID_SOCKET;
    }
}

std::shared_ptr<IAsyncResult> Socket::BeginConnect(
    const struct sockaddr* name,
    int namelen,
    const ASYNCCALLBACK& Callback,
    DWORD_PTR Context
)
{
    using namespace std;

    auto ai = shared_ptr<SocketAsyncResultImpl>(
        new SocketAsyncResultImpl(*this, SocketAsyncResultImpl::Connect, Context, Callback)
    );

    BOOL ret = Socket::pfnConnectEx(s, name, namelen, nullptr, 0, nullptr, ai.get());
    int code = WSAGetLastError();
    if (ret)
    {
        ai->Complete(static_cast<DWORD>(code), true);
    }
    else if (!ret && code != WSA_IO_PENDING)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        ai.reset();
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return static_pointer_cast<IAsyncResult>(ai);
}

inline void Socket::EndConnect(std::shared_ptr<IAsyncResult> asyncResult)
{
    std::static_pointer_cast<SocketAsyncResultImpl>(asyncResult)->WaitForCompletion();
}

std::shared_ptr<IAsyncResult> Socket::BeginDisconnect(const ASYNCCALLBACK& Callback, DWORD_PTR Context)
{
    using namespace std;

    auto ai = shared_ptr<SocketAsyncResultImpl>(
        new SocketAsyncResultImpl(*this, SocketAsyncResultImpl::DisConnect, Context, Callback)
    );

    BOOL ret = Socket::pfnDisconnectEx(s, ai.get(), TF_REUSE_SOCKET, 0);
    int code = WSAGetLastError();
    if (ret)
    {
        ai->Complete(code, true);
    }
    else if (!ret && code != WSA_IO_PENDING)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        ai.reset();
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return static_pointer_cast<IAsyncResult>(ai);
}

inline void Socket::EndDisconnect(std::shared_ptr<IAsyncResult> asyncResult)
{
    std::static_pointer_cast<SocketAsyncResultImpl>(asyncResult)->WaitForCompletion();
}

WSAEVENT Socket::Listen(int backlog)
{
    CHAR Err[100] = {};

    WSAEVENT hEventAccept = ::WSACreateEvent();
    int code = WSAGetLastError();
    if (hEventAccept == WSA_INVALID_EVENT)
    {
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    int retval = ::WSAEventSelect(s, hEventAccept, FD_ACCEPT);
    if (retval != 0)
    {
        code = WSAGetLastError();
        WSACloseEvent(hEventAccept);
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }

    retval = ::listen(this->s, backlog);
    if (retval != 0)
    {
        code = WSAGetLastError();
        WSACloseEvent(hEventAccept);
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, WSAGetLastError());
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return hEventAccept;
}

DWORD Socket::IoControl(
    _In_  DWORD dwIoControlCode,
    _In_  LPVOID lpvInBuffer,
    _In_  DWORD cbInBuffer,
    _Out_ LPVOID lpvOutBuffer,
    _In_  DWORD cbOutBuffer
)
{
    DWORD cbBytesReturned = 0;
    int retval = WSAIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, &cbBytesReturned, nullptr, nullptr);
    if (retval == 0)
    {
        return cbBytesReturned;
    }
    else
    {
        int code = WSAGetLastError();
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
}

std::shared_ptr<IAsyncResult> Socket::BeginReceive(
    LPBYTE pBuffer,
    DWORD cbBuffer,
    LPDWORD lpdwFlags,
    const ASYNCCALLBACK& Callback,
    DWORD_PTR Context
)
{
    using namespace std;
    
    auto ai = shared_ptr<SocketAsyncResultImpl>(
        new SocketAsyncResultImpl(*this, SocketAsyncResultImpl::Receive, Context, Callback)
    );
    ai->get__InternalParams().push_back(reinterpret_cast<DWORD_PTR>(lpdwFlags));

    WSABUF buf = { cbBuffer, reinterpret_cast<char*>(pBuffer) };
    DWORD dwNumberOfBytesRecvd = 0;
    int retval = ::WSARecv(s, &buf, 1, &dwNumberOfBytesRecvd, lpdwFlags, ai.get(), nullptr);
    int code = WSAGetLastError();
    if (retval == 0)
    {
        ai->Complete(code, true);
    }
    else if (code != WSA_IO_PENDING)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        ai.reset();
        UpdateStatusAfterSocketError(code);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return static_pointer_cast<IAsyncResult>(ai);
}

DWORD Socket::EndReceive(std::shared_ptr<IAsyncResult> asyncResult)
{
    auto async = std::static_pointer_cast<SocketAsyncResultImpl>(asyncResult);
    async->WaitForCompletion();

    DWORD cbTransfer = 0, dwFlags = 0;
    ::WSAGetOverlappedResult(s, async.get(), &cbTransfer, TRUE, &dwFlags);
    return cbTransfer;
}

std::shared_ptr<IAsyncResult> Socket::BeginSend(
    const LPBYTE lpBuffer,
    DWORD cbBuffer,
    DWORD dwFlags,
    const ASYNCCALLBACK & Callback,
    DWORD_PTR Context
)
{
    using namespace std;
    auto ai = shared_ptr<SocketAsyncResultImpl>(new SocketAsyncResultImpl(*this, SocketAsyncResultImpl::Send, Context, Callback));

    WSABUF buf = { cbBuffer, reinterpret_cast<char*>(lpBuffer) };
    int retval = ::WSASend(s, &buf, 1, nullptr, dwFlags, ai.get(), nullptr);
    int code = WSAGetLastError();
    if (retval == 0)
    {
        ai->Complete(code, true);
    }
    else if (code != WSA_IO_PENDING)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        ai.reset();
        UpdateStatusAfterSocketError(code);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return static_pointer_cast<IAsyncResult>(ai);
}

DWORD Socket::EndSend(std::shared_ptr<IAsyncResult> asyncResult)
{
    auto async = std::static_pointer_cast<SocketAsyncResultImpl>(asyncResult);
    async->WaitForCompletion();

    DWORD cbTransfer = 0, dwFlags = 0;
    ::WSAGetOverlappedResult(s, async.get(), &cbTransfer, TRUE, &dwFlags);
    return cbTransfer;
}

void Socket::Shutdown(int how)
{
    int ret = ::shutdown(s, how);
    int code = WSAGetLastError();
    if (ret != 0)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
}

std::shared_ptr<IAsyncResult> Socket::BeginSendTo(
    const BYTE* lpBuffer,
    DWORD cbBuffer,
    DWORD dwFlags,
    const sockaddr * lpTo,
    int iToLen,
    const ASYNCCALLBACK& Callback,
    DWORD_PTR Context
)
{
    using namespace std;

    auto ai = shared_ptr<SocketAsyncResultImpl>(
        new SocketAsyncResultImpl(*this, SocketAsyncResultImpl::Send, Context, Callback)
    );

    WSABUF buf = { cbBuffer, (char*)lpBuffer };
    int retval = ::WSASendTo(s, &buf, 1, nullptr, 0, lpTo, iToLen, ai.get(), nullptr);
    int code = WSAGetLastError();
    if (retval == 0)
    {
        ai->Complete(code, true);
    }
    else if (code != WSA_IO_PENDING)
    {
        CHAR Err[100] = {};
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        ai.reset();
        UpdateStatusAfterSocketError(code);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }
    return static_pointer_cast<IAsyncResult>(ai);
}

DWORD Socket::EndSendTo(std::shared_ptr<IAsyncResult> asyncResult)
{
    auto async = std::static_pointer_cast<SocketAsyncResultImpl>(asyncResult);
    async->WaitForCompletion();

    DWORD cbTransfer = 0, dwFlags = 0;
    ::WSAGetOverlappedResult(s, async.get(), &cbTransfer, TRUE, &dwFlags);
    return cbTransfer;
}

bool Socket::Poll(int microSeconds, SelectMode mode)
{
    if (microSeconds < -1)
        throw ArgumentException();

    fd_set set = { 1, { s } };
    int num = 0;
    if (microSeconds != -1)
    {
        timeval time = { microSeconds / 10000000L, microSeconds % 10000000L };
        num = ::select(0, mode == SelectMode::SelectRead ? &set : nullptr, mode == SelectMode::SelectWrite ? &set : nullptr, mode == SelectMode::SelectError ? &set : nullptr, &time);
    }
    else
    {
        num = ::select(0, mode == SelectMode::SelectRead ? &set : nullptr, mode == SelectMode::SelectWrite ? &set : nullptr, mode == SelectMode::SelectError ? &set : nullptr, nullptr);
    }

    if (num == -1)
    {
        CHAR Err[100] = {};
        int code = ::WSAGetLastError();
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAGetLastError()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }

    return set.fd_count != 0 && set.fd_array[0] == s;
}

LPFN_ACCEPTEX Socket::pfnAcceptEx;
LPFN_CONNECTEX Socket::pfnConnectEx;
LPFN_DISCONNECTEX Socket::pfnDisconnectEx;
LPFN_GETACCEPTEXSOCKADDRS Socket::pfnGetAcceptExSockaddrs;

/************************************************************************/
/*初始化WinSock库，初始化创建内存池、线程池并调用初始化函数*/
/*取得AcceptEx等函数的指针*/
/************************************************************************/
void Socket::InitializeWinsock(_In_ WORD wVersionRequested, _Out_ LPWSADATA lpWSAData)
{
    int code = ::WSAStartup(wVersionRequested, lpWSAData); CHAR Err[100] = {};
    if (code != 0)
    {
        StringCchPrintfA(Err, 100, "%s() failed.\r\nWSAStartup()==%i\r\n", __FUNCTION__, code);
        ::OutputDebugStringA(Err);
        SocketException e(code);
        SET_EXCEPTION(e);
        throw e;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    DWORD dwBytes;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    ::WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx, sizeof(GUID),
        &pfnAcceptEx, sizeof(LPFN_ACCEPTEX),
        &dwBytes, nullptr, nullptr);
    GUID GuidGetAcceptExSocketAddrs = WSAID_GETACCEPTEXSOCKADDRS;
    ::WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidGetAcceptExSocketAddrs, sizeof(GUID),
        &pfnGetAcceptExSockaddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS),
        &dwBytes, nullptr, nullptr);
    GUID GuidConnectEx = WSAID_CONNECTEX;
    ::WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidConnectEx, sizeof(GUID),
        &pfnConnectEx, sizeof(LPFN_CONNECTEX),
        &dwBytes, nullptr, nullptr);
    GUID GuidDisconnectEx = WSAID_DISCONNECTEX;
    ::WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidDisconnectEx, sizeof(GUID),
        &pfnDisconnectEx, sizeof(LPFN_DISCONNECTEX),
        &dwBytes, nullptr, nullptr);
    closesocket(s);
}

/************************************************************************/
/*销毁内存池与线程池，释放WinSock库*/
/************************************************************************/
void Socket::UninitializeWinsock()
{
    WSACleanup();
}

void Socket::UpdateStatusAfterSocketError(int errorCode)
{
    if (_Connected && (s != INVALID_SOCKET || (errorCode != WSAEWOULDBLOCK && errorCode != WSA_IO_PENDING && errorCode != WSAENOBUFS && errorCode != WSAETIMEDOUT)))
    {
        _Connected = false;
    }
}

#pragma endregion

//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region TcpClient

    hxc::Critical_Section<std::map<SOCKET, Socket*>> AcceptSockets;

    TcpClient::TcpClient(int AddressFamily) : _Socket(Socket::Create(AddressFamily, SOCK_STREAM, IPPROTO_TCP))
    {
        sockaddr_in sock = {};
        sock.sin_family = _Socket->get__AddressFamily();
        //sock.sin_addr.S_un.S_addr = INADDR_ANY;
        _Socket->Bind(reinterpret_cast<sockaddr*>(&sock), sizeof(sockaddr_in));
    }

    TcpClient::~TcpClient()
    {
        Close();
    }

    std::shared_ptr<Socket> TcpClient::get__Client() { return _Socket; }

    bool TcpClient::get__Connected()
    {
        return _Socket->get__Connected();
    }

    void TcpClient::Close(void)
    {
        ASYNCCALLBACK callback = [](std::shared_ptr<IAsyncResult> pi)
        {
            
        };
        if (_Socket->get__Connected())
        {
            /*IAsyncResult* pi = _Socket.Disconnect(callback);
            WaitForSingleObject(pi->get__AsyncWaitHandle(), INFINITE);
            CloseHandle(pi->get__AsyncWaitHandle());
            delete pi;*/
        }
        _Socket->CloseSocket();
    }

    Task TcpClient::ConnectAsync(
        const std::wstring& RemoteIP,
        USHORT RemotePort
    )
    {
        sockaddr_in addr = {};
        addr.sin_family = _Socket->get__AddressFamily();
        addr.sin_port = htons(RemotePort);
        InetPtonW(addr.sin_family, RemoteIP.c_str(), &addr.sin_addr);
        auto ai =  _Socket->BeginConnect((sockaddr*)&addr, sizeof(sockaddr_in), nullptr, NULL);
        auto endMethod = std::bind(&Socket::EndConnect, _Socket, std::placeholders::_1);
        return Task::FromAsync(ai, endMethod);
    }

    Task TcpClient::SendAsync(
        const LPBYTE lpBuffer,
        DWORD cbBuffer,
        DWORD dwFlags
    )
    {
        auto ai = _Socket->BeginSend(lpBuffer, cbBuffer, dwFlags, nullptr, NULL);
        auto endMethod = [this](std::shared_ptr<IAsyncResult> async)
        {
            DWORD_PTR ret = static_cast<DWORD_PTR>(_Socket->EndSend(async));
            return ret;
        };
        return Task::FromAsync1(ai, endMethod);
    }

    Task TcpClient::ReceiveAsync(
        LPBYTE pBuffer,
        DWORD cbBuffer,
        LPDWORD lpdwFlags
    )
    {
        auto ai = _Socket->BeginReceive(pBuffer, cbBuffer, lpdwFlags, nullptr, NULL);
        auto endMethod = [this](std::shared_ptr<IAsyncResult> async)
        {
            return static_cast<DWORD_PTR>(_Socket->EndReceive(async));
        };
        return Task::FromAsync1(ai, endMethod);
    }

//DWORD WINAPI CTcpSocket::ListenThread(PVOID pParam)
//{
//    CTcpSocket* pThis = (CTcpSocket*)pParam;
//    while(true)
//    {
//        DWORD dwWaitRet = ::WSAWaitForMultipleEvents(1, &pThis->hEventAccept, TRUE, INFINITE, TRUE);
//        if (dwWaitRet == WSA_WAIT_EVENT_0)
//        {
//            pThis->PostAcceptOperation();
//            WSAResetEvent(pThis->hEventAccept);
//        }
//        else if (dwWaitRet == WSA_WAIT_FAILED)
//        {
//            return 0;
//        }
//    }
//}


    CTcpListener::CTcpListener(void)
    {
    }

    SocketException::SocketException(int WSAErrorCode) : Exception(HRESULT_FROM_WIN32(WSAErrorCode))
    {
        _MessageW = Win32Exception::FormatErrorMessage(static_cast<DWORD>(WSAErrorCode));
    }

    tcp_stream::tcp_stream(const tcp_stream & o)
    {
        _socket = o._socket;
    }

    tcp_stream::tcp_stream(std::shared_ptr<Socket> socket) :
        _socket(socket)
    {
    }

    tcp_stream::~tcp_stream()
    {
    }

    tcp_stream & tcp_stream::operator=(const tcp_stream & o)
    {
        if (this != &o)
        {
            _socket = o._socket;
        }
        return *this;
    }

    int tcp_stream::Read(LPBYTE buffer, int offset, int count)
    {
        Task t = ReadAsync(buffer, offset, count);
        t.Wait();
        return t.get__Result();
    }

    Task tcp_stream::ReadAsync(LPBYTE buffer, int offset, int count)
    {
        LPDWORD lpdwFlags = new DWORD(0);
        auto ai = _socket->BeginReceive(&buffer[offset], count, lpdwFlags, nullptr, NULL);
        auto endMethod = [this, lpdwFlags](std::shared_ptr<IAsyncResult> async)
        {
            DWORD_PTR ret = static_cast<DWORD_PTR>(_socket->EndReceive(async));
            delete lpdwFlags;
            return ret;
        };
        return Task::FromAsync1(ai, endMethod);
    }

    int tcp_stream::Write(const LPBYTE buffer, int offset, int count)
    {
        Task t = WriteAsync(buffer, offset, count);
        t.Wait();
        return t.get__Result();
    }

    Task tcp_stream::WriteAsync(const LPBYTE buffer, int offset, int count)
    {
        DWORD dwFlags = 0;
        auto ai = _socket->BeginSend(&buffer[offset], count, dwFlags, nullptr, NULL);
        auto endMethod = [this](std::shared_ptr<IAsyncResult> async)
        {
            return static_cast<DWORD_PTR>(_socket->EndSend(async));
        };
        return Task::FromAsync1(ai, endMethod);
    }

};	//namespace hxc
