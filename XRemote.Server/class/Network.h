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
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

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
            _buffer_send = reinterpret_cast<char_type*>(_DataPool::BufferPool().Pop());
            ZeroMemory(_buffer_send, BUFFER_SIZE);
            setp(_buffer_send, _buffer_send + BUFFER_SIZE / sizeof(char_type));

            _buffer_recv = reinterpret_cast<char_type*>(_DataPool::BufferPool().Pop());
            ZeroMemory(_buffer_recv, BUFFER_SIZE);
            setg(_buffer_recv, _buffer_recv + BUFFER_SIZE / sizeof(char_type), _buffer_recv + BUFFER_SIZE / sizeof(char_type));
        }
        virtual ~tcp_streambuf()
        {
            sync();

            _DataPool::BufferPool().Push(reinterpret_cast<LPBYTE>(_buffer_send));
            _DataPool::BufferPool().Push(reinterpret_cast<LPBYTE>(_buffer_recv));
        }
    protected:
        virtual int_type overflow(int_type ch = traits_type::eof())
        {
            flush();
            setp(pbase(), epptr()); //重新设定send缓冲, 将pptr()重置到pbase()处

            if (traits_type::eq_int_type(traits_type::eof(), ch))
                return traits_type::not_eof(ch);

            sputc(traits_type::to_char_type(ch));  //put c into buffer again
            return ch;
        }

        virtual int_type underflow()
        {
            //此时get buffer中已经没有内容, 重新读入
            DWORD dwFlags = 0;
            Task t = _client.ReceiveAsync(reinterpret_cast<LPBYTE>(eback()), BUFFER_SIZE, &dwFlags);
            t.Wait(INFINITE);
            int recv_size = t.get_Result();

            if (recv_size == 0)
                return traits_type::to_int_type(traits_type::eof());

            setg(eback(), eback(), eback() + recv_size / sizeof(char_type));
            return traits_type::to_int_type(*gptr());
        }

        virtual int sync()
        {
            flush();
            return 0;
        }
    private:
        int flush(void)
        {
            _client.SendAsync(reinterpret_cast<LPBYTE>(pbase()), (pptr() - pbase() + 1) * sizeof(char_type), 0).Wait(INFINITE);

            setp(pbase(), epptr());

            return 0;
        }
    private:
        const int BUFFER_SIZE = _DataPool::BufferSize;
        char_type* _buffer_recv;
        char_type* _buffer_send;

        TcpClient& _client;
    };

    template<typename _Elem>
    class tcp_stream : public std::basic_iostream<_Elem, std::char_traits<_Elem>>
    {
    public:
        explicit tcp_stream(TcpClient& client)
            : streambuf(client), std::basic_iostream<char_type, traits_type>(&streambuf)
        {

        }

        virtual ~tcp_stream()
        {

        }

        tcp_stream* rdbuf()
        {
            return &streambuf;
        }
    private:
        tcp_streambuf<char_type> streambuf;
    };

};//namespace hxc

#endif //#if !defined _HXC_NETWORK_H_