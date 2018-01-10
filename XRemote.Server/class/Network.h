#pragma once

#if !defined _HXC_NETWORK_H_
#define _HXC_NETWORK_H_

namespace hxc
{
    class SocketException : public Exception
    {
    public:
        explicit SocketException(int WSAErrorCode);
    };

    enum SelectMode
    {
        SelectError,
        SelectRead,
        SelectWrite
    };

    class Socket
    {
        friend class SocketAsyncResultImpl;
    protected:
        SOCKET s;
        Socket();
    public:
#pragma region Properties
        int get__AddressFamily();
        bool get__Connected();
#pragma endregion

#pragma region Methods
        static std::shared_ptr<Socket> Create();

        std::shared_ptr<IAsyncResult> BeginAccept(
            _In_ std::shared_ptr<Socket> AcceptSocket,
            _In_ DWORD receiveSize,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        std::shared_ptr<Socket> EndAccept(
            std::unique_ptr<BYTE> Buffer,
            LPDWORD pReceiveSize,
            std::shared_ptr<IAsyncResult> asyncResult
        );

        void Bind(const struct sockaddr* name, int namelen);

        void CloseSocket(void);

        std::shared_ptr<IAsyncResult> BeginConnect(
            _In_ const struct sockaddr* name,
            _In_ int namelen,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        void EndConnect(IAsyncResult* asyncResult);

        std::shared_ptr<IAsyncResult> BeginDisconnect(_In_ const ASYNCCALLBACK& Callback, _In_ DWORD_PTR Context);

        void EndDisconnect(IAsyncResult* asyncResult);

        WSAEVENT Listen(_In_ int backlog = SOMAXCONN);

        DWORD IoControl(
            _In_  DWORD dwIoControlCode,
            _In_  LPVOID lpvInBuffer,
            _In_  DWORD cbInBuffer,
            _Out_ LPVOID lpvOutBuffer,
            _In_  DWORD cbOutBuffer
        );

        std::shared_ptr<IAsyncResult> BeginReceive(
            _In_ LPBYTE pBuffer,
            _In_ DWORD cbBuffer,
            _Out_ LPDWORD lpdwFlags,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        DWORD EndReceive(std::shared_ptr<IAsyncResult> asyncResult);

        IAsyncResult* BeginReceiveFrom(
            _Inout_ LPBYTE lpBuffers,
            _In_ DWORD dwBufferLen,
            _Inout_ LPDWORD lpFlags,
            _Out_ struct sockaddr *lpFrom,
            _Inout_ LPINT lpFromlen
        );

        DWORD EndReceiveFrom(IAsyncResult* asyncResult);

        std::shared_ptr<IAsyncResult> BeginSend(
            _In_ const std::vector<BYTE>& Buffer,
            _In_ DWORD dwFlags,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        DWORD EndSend(std::shared_ptr<IAsyncResult> asyncResult);

        std::shared_ptr<IAsyncResult> BeginSendTo(
            const BYTE* pBuffer,
            DWORD cbBuffer,
            DWORD dwFlags,
            const sockaddr* lpTo,
            int iToLen,
            const ASYNCCALLBACK& Callback,
            DWORD_PTR Context
        );

        DWORD EndSendTo(std::shared_ptr<IAsyncResult> asyncResult);

        bool Poll(int microSeconds, SelectMode mode);

        void Shutdown(int how);
#pragma endregion

        operator SOCKET() { return s; }
        static void InitializeWinsock(_In_ WORD wVersionRequested, _Out_ LPWSADATA lpWSAData);
        static void UninitializeWinsock();

    private:
        Socket(const Socket&) { throw NotImplementedException(); }
        Socket& operator=(const Socket&) { throw NotImplementedException(); }
        void UpdateStatusAfterSocketError(int errorCode);
#pragma region PropertiesInternal
        int _AddressFamily;
        volatile long _Connected;
#pragma endregion
        static LPFN_ACCEPTEX pfnAcceptEx;
        static LPFN_CONNECTEX pfnConnectEx;
        static LPFN_DISCONNECTEX pfnDisconnectEx;
        static LPFN_GETACCEPTEXSOCKADDRS pfnGetAcceptExSockaddrs;
    };

    class TcpClient
    {
    public:
        TcpClient();
        ~TcpClient();
    public:
#pragma region Properties
        std::shared_ptr<Socket> get__Client();
        bool get__Connected();
#pragma endregion
#pragma region Methods
        void Close(void);

        Task ConnectAsync(
            _In_ const std::wstring& RemoteIP,
            _In_ USHORT RemotePort
        );

        Task SendAsync(
            _In_ const std::vector<BYTE>& Buffer,
            _In_ DWORD dwFlags
        );

        Task ReceiveAsync(
            _In_ LPBYTE pBuffer,
            _Inout_ DWORD cbBuffer,
            _Out_ LPDWORD lpdwFlags
        );
#pragma endregion
    private:
#pragma region PropertiesInternal
        std::shared_ptr<Socket> _Socket;
#pragma endregion
        CRITICAL_SECTION Lock;
    };

    class CTcpListener
    {
    public:
        CTcpListener(void);
    };

    class __declspec(novtable) SocketLayerBase
    {
    public:
        virtual void OnSendData(std::vector<BYTE>& data) = 0;
        virtual void OnReceiveData(std::vector<BYTE>& data) = 0;
        virtual void OnShutdown() = 0;
    };

    class SafeLayer : public SocketLayerBase
    {
    public:
        SafeLayer();
        ~SafeLayer();

        void set__PublicKey_Remote(const std::vector<BYTE>& value);
        std::vector<BYTE> get__PublicKet_Remote();

        void set__Certificate(const X509Certificate& value);
        X509Certificate get__Certificate();

        virtual void OnSendData(std::vector<BYTE>& data);
        virtual void OnReceiveData(std::vector<BYTE>& data);
        virtual void OnShutdown() {}
    };

    template<typename _Elem>
    class tcp_streambuf : public std::basic_streambuf<_Elem, std::char_traits<_Elem>>
    {
    public:
        explicit tcp_streambuf(TcpClient& client) : _client(client)
        {
            _buffer_send.reset(new tcp_streambuf::char_type[BUFFER_SIZE]);
            ZeroMemory(_buffer_send, BUFFER_SIZE * sizeof(typename tcp_streambuf::char_type));
            setp(_buffer_send, _buffer_send + BUFFER_SIZE - 1);

            _buffer_recv.reset(new tcp_streambuf::char_type[BUFFER_SIZE]);
            ZeroMemory(_buffer_recv, BUFFER_SIZE * sizeof(typename tcp_streambuf::char_type));
            setg(_buffer_recv, _buffer_recv + BUFFER_SIZE - 1);
        }
        virtual ~tcp_streambuf()
        {
            sync();
        }
    protected:
        virtual int_type overflow(int_type ch)
        {
            if (ch != _Traits::eof())
            {
                *pptr() = ch;
                pbump(1);
            }

            if (flush() == _Traits::eof())
                return _Traits::eof();
            return _Traits::not_eof();
        }

        virtual int sync()
        {
            if (flush() == _Traits::eof())
                return -1;
            return basic_streambuf::sync();
        }
    private:
        int flush(void)
        {
            using namespace std;
            vector<BYTE> data(
                pbase(),
                sizeof(tcp_streambuf::char_type) == sizeof(wchar_t) ?
                    reinterpret_cast<BYTE*>((int)pptr() + 1) :
                    pptr()
            );
            _client.SendAsync(data, 0).Wait(INFINITE);
        }
    private:
        const int BUFFER_SIZE = 256;
        std::unique_ptr<typename tcp_streambuf::char_type> _buffer_recv;
        std::unique_ptr<typename tcp_streambuf::char_type> _buffer_send;

        TcpClient& _client;
    };

};//namespace hxc

#endif //#if !defined _HXC_NETWORK_H_