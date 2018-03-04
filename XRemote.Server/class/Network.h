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
        Socket(int af, int type, int protocol);
    public:
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

#pragma region Properties
        int get__AddressFamily();
        bool get__Connected();
#pragma endregion

#pragma region Methods
        static std::shared_ptr<Socket> Create(int af, int type, int protocol);

        std::shared_ptr<IAsyncResult> BeginAccept(
            _In_ std::shared_ptr<Socket> AcceptSocket,
            _In_ DWORD receiveSize,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        std::shared_ptr<Socket> EndAccept(
            LPBYTE* ppBuffer,
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

        inline void EndConnect(std::shared_ptr<IAsyncResult> asyncResult);

        std::shared_ptr<IAsyncResult> BeginDisconnect(_In_ const ASYNCCALLBACK& Callback, _In_ DWORD_PTR Context);

        inline void EndDisconnect(std::shared_ptr<IAsyncResult> asyncResult);

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

        std::shared_ptr<IAsyncResult> BeginReceiveFrom(
            _Inout_ LPBYTE lpBuffers,
            _In_ DWORD dwBufferLen,
            _Inout_ LPDWORD lpFlags,
            _Out_ struct sockaddr *lpFrom,
            _Inout_ LPINT lpFromlen
        );

        DWORD EndReceiveFrom(std::shared_ptr<IAsyncResult> asyncResult);

        std::shared_ptr<IAsyncResult> BeginSend(
            _In_ const LPBYTE lpBuffer,
            _In_ DWORD cbBuffer,
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

        SOCKET get__Handle() { return s; }
        static void InitializeWinsock(_In_ WORD wVersionRequested, _Out_ LPWSADATA lpWSAData);
        static void UninitializeWinsock();

    private:
        void UpdateStatusAfterSocketError(int errorCode);
#pragma region PropertiesInternal
        int _AddressFamily;
        int _SocketType;
        int _ProtocolType;
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
        TcpClient(int AddressFamily);
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
            _In_ const LPBYTE lpBuffer,
            _In_ DWORD cbBuffer,
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

    class tcp_stream
    {
    public:
        tcp_stream(const tcp_stream& o);
        explicit tcp_stream(std::shared_ptr<Socket> socket);
        virtual ~tcp_stream();
    public:
        tcp_stream & operator=(const tcp_stream& o);
        int Read(LPBYTE buffer, int offset, int count);
        Task ReadAsync(LPBYTE buffer, int offset, int count);
        int Write(const LPBYTE buffer, int offset, int count);
        Task WriteAsync(const LPBYTE buffer, int offset, int count);
    private:
        std::shared_ptr<Socket> _socket;
    };

};//namespace hxc

#endif //#if !defined _HXC_NETWORK_H_