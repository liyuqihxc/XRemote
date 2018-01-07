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
    public:
        Socket();
    public:
#pragma region Properties
        int get__AddressFamily();
        bool get__Connected();
#pragma endregion

#pragma region Methods
        IAsyncResult* BeginAccept(
            _In_ Socket& AcceptSocket,
            _In_ DWORD receiveSize,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        Socket& EndAccept(LPBYTE* ppArray, LPDWORD pReceiveSize, IAsyncResult* asyncResult);

        void Bind(const struct sockaddr* name, int namelen);

        void CloseSocket(void);

        IAsyncResult* BeginConnect(
            _In_ const struct sockaddr* name,
            _In_ int namelen,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        void EndConnect(IAsyncResult* asyncResult);

        IAsyncResult* BeginDisconnect(_In_ const ASYNCCALLBACK& Callback, _In_ DWORD_PTR Context);

        void EndDisconnect(IAsyncResult* asyncResult);

        WSAEVENT Listen(_In_ int backlog = SOMAXCONN);

        DWORD IoControl(
            _In_  DWORD dwIoControlCode,
            _In_  LPVOID lpvInBuffer,
            _In_  DWORD cbInBuffer,
            _Out_ LPVOID lpvOutBuffer,
            _In_  DWORD cbOutBuffer
        );

        IAsyncResult* BeginReceive(
            _In_ LPBYTE pBuffer,
            _In_ DWORD cbBuffer,
            _Out_ LPDWORD lpdwFlags,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        DWORD EndReceive(IAsyncResult* asyncResult);

        IAsyncResult* BeginReceiveFrom(
            _Inout_ LPBYTE lpBuffers,
            _In_ DWORD dwBufferLen,
            _Inout_ LPDWORD lpFlags,
            _Out_ struct sockaddr *lpFrom,
            _Inout_ LPINT lpFromlen
        );

        DWORD EndReceiveFrom(IAsyncResult* asyncResult);

        IAsyncResult* BeginSend(
            _In_ const BYTE* lpBuffer,
            _In_ DWORD cbBuffer,
            _In_ DWORD dwFlags,
            _In_ const ASYNCCALLBACK& Callback,
            _In_ DWORD_PTR Context
        );

        DWORD EndSend(IAsyncResult* asyncResult);

        IAsyncResult* BeginSendTo(
            const BYTE* pBuffer,
            DWORD cbBuffer,
            DWORD dwFlags,
            const sockaddr* lpTo,
            int iToLen,
            const ASYNCCALLBACK& Callback,
            DWORD_PTR Context
        );

        DWORD EndSendTo(IAsyncResult* asyncResult);

        bool Poll(int microSeconds, SelectMode mode);

        void Shutdown(int how);
#pragma endregion

        inline operator SOCKET();
        static void InitializeWinsock(_In_ WORD wVersionRequested, _Out_ LPWSADATA lpWSAData);
        static void UninitializeWinsock();

    private:
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
        Socket& get__Client();
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
            _In_ DWORD dwBufferLen,
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
        Socket _Socket;
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


};//namespace hxc

#endif //#if !defined _HXC_NETWORK_H_