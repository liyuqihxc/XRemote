#include "stdafx.h"
#include "hxc.h"
#include "ClassFactoryImpl.h"

ZeroCopyNetworkInputStream::ZeroCopyNetworkInputStream(hxc::tcp_stream & tcp_stream, int32_t capacity) :
    _buffer((uint8_t*)malloc(capacity)), _capacity(capacity), _tcp_stream(tcp_stream), _signal(CreateEvent(nullptr, true, false, nullptr)),
    _head(0), _ptr(0), _recv_task(hxc::Task::BindFunction(&ZeroCopyNetworkInputStream::ReceiveProc, this), NULL)
{
    _recv_task.Start();
}

ZeroCopyNetworkInputStream::~ZeroCopyNetworkInputStream()
{
    _recv_task.Cancel();

    if (_capacity)
        free(_buffer);
}

bool ZeroCopyNetworkInputStream::Next(const void ** data, int * size)
{
    _head = _ptr;
    ::ResetEvent(_signal);

    if (::WaitForSingleObject(_signal, INFINITE) == WAIT_FAILED)
        return false;

    *data = &_buffer[_head];
    *size = _ptr - _head;
    return true;
}

void ZeroCopyNetworkInputStream::BackUp(int count)
{
    // NetworkStream��֧��BackUp����
    hxc::NotSupportedException e;
    SET_EXCEPTION(e);
    throw e;
}

bool ZeroCopyNetworkInputStream::Skip(int count)
{
    // NetworkStream��֧��Skip����
    hxc::NotSupportedException e;
    SET_EXCEPTION(e);
    throw e;
}

long long ZeroCopyNetworkInputStream::ByteCount() const
{
    // NetworkStream��֧��ByteCount����
    hxc::NotSupportedException e;
    SET_EXCEPTION(e);
    throw e;
}

DWORD_PTR ZeroCopyNetworkInputStream::ReceiveProc(DWORD_PTR Param, HANDLE hCancel)
{
    HANDLE signals[] = { hCancel, _signal };
    DWORD wait_result = 0;
    while (wait_result = ::WaitForMultipleObjects(2, signals, false, 0))
    {
        if (wait_result == WAIT_OBJECT_0) // Task ȡ��
            break;
        else if (wait_result == WAIT_FAILED)
            return -1;
        else if (wait_result == WAIT_OBJECT_0 + 1) // ZeroCopyNetworkInputStream::_signal���źŵ�״̬ʱ�ſ��Խ�������
            continue;

        if (_head == _ptr && _ptr == _capacity)
            _ptr = _head = 0;
        else if (_ptr == _capacity)
            continue;

        _ptr += _tcp_stream.Read(_buffer, _ptr, _capacity - _ptr);
        ::SetEvent(_signal);
    }
    return 0;
}

bool ZeroCopyNetworkInputStream::ReadDelimitedFrom(google::protobuf::io::ZeroCopyInputStream * rawInput, google::protobuf::MessageLite & message)
{
    // We create a new coded stream for each message.  Don't worry, this is fast,
    // and it makes sure the 64MB total size limit is imposed per-message rather
    // than on the whole stream.  (See the CodedInputStream interface for more
    // info on this limit.)
    google::protobuf::io::CodedInputStream input(rawInput);

    // Read the size.
    uint32_t size;
    if (!input.ReadVarint32(&size))
        return false;

    // Tell the stream not to read beyond that size.
    google::protobuf::io::CodedInputStream::Limit limit =
        input.PushLimit(size);

    // Parse the message.
    if (!message.MergeFromCodedStream(&input))
        return false;
    if (!input.ConsumedEntireMessage())
        return false;

    // Release the limit.
    input.PopLimit(limit);

    return true;
}

ZeroCopyNetworkOutputStream::ZeroCopyNetworkOutputStream(hxc::tcp_stream & tcp_stream, int32_t capacity):
    _buffer((uint8_t*)malloc(capacity)), _capacity(capacity), _tcp_stream(tcp_stream), _ptr(0), _allow_backup(false)
{
}

ZeroCopyNetworkOutputStream::~ZeroCopyNetworkOutputStream()
{
    if (_capacity)
        free(_buffer);
}

bool ZeroCopyNetworkOutputStream::Next(void ** data, int * size)
{
    if (_ptr != 0)
    {
        _tcp_stream.Write(reinterpret_cast<LPBYTE>(_buffer), 0, _ptr);
        _ptr = 0;
    }
    *data = _buffer;
    *size = _capacity;
    _ptr = _capacity;
    _allow_backup = true;
    return true;
}

void ZeroCopyNetworkOutputStream::BackUp(int count)
{
    if (!_allow_backup)
        return;
    _ptr = (std::min)(0, _ptr - count);
    _allow_backup = _ptr == 0;
}

long long ZeroCopyNetworkOutputStream::ByteCount() const
{
    // NetworkStream��֧��ByteCount����
    hxc::NotSupportedException e;
    SET_EXCEPTION(e);
    throw e;
}

bool ZeroCopyNetworkOutputStream::WriteAliasedRaw(const void * data, int size)
{
    return _tcp_stream.Write((const LPBYTE)data, 0, size) == size;
}

bool ZeroCopyNetworkOutputStream::WriteDelimitedTo(const google::protobuf::MessageLite & message, google::protobuf::io::ZeroCopyOutputStream * rawOutput)
{
    // We create a new coded stream for each message.  Don't worry, this is fast.
    google::protobuf::io::CodedOutputStream output(rawOutput);

    // Write the size.
    const int size = message.ByteSize();
    output.WriteVarint32(size);

    uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
    if (buffer != NULL) {
        // Optimization:  The message fits in one buffer, so use the faster
        // direct-to-array serialization path.
        message.SerializeWithCachedSizesToArray(buffer);
    }
    else {
        // Slightly-slower path when the message is multiple buffers.
        message.SerializeWithCachedSizes(&output);
        if (output.HadError()) return false;
    }

    return true;
}

ClassFactoryImpl::ClassFactoryImpl() :
    _MaintainConnectionTask(hxc::Task::BindFunction(&ClassFactoryImpl::MaintainConnection, this), NULL, WT_EXECUTEINLONGTHREAD),
    _Connection(AF_INET)
{
}

STDMETHODIMP ClassFactoryImpl::CreateInstance(REFIID riid, IUnknown ** ppvObject)
{
    return E_NOTIMPL;
}

STDMETHODIMP ClassFactoryImpl::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}

HRESULT ClassFactoryImpl::FinalConstruct()
{
    _MaintainConnectionTask.Start();
    return S_OK;
}

void ClassFactoryImpl::FinalRelease()
{
    _MaintainConnectionTask.Cancel();
    _Connection.Close();
}

DWORD_PTR ClassFactoryImpl::MaintainConnection(DWORD_PTR Param, HANDLE hCancel)
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

        hxc::Task ReceiveTask(hxc::Task::BindFunction(&ClassFactoryImpl::OnReceive, this), NULL, WT_EXECUTEINLONGTHREAD);
        ReceiveTask.Start();

        SOCKET s = _Connection.get__Client()->get__Handle();
        WSAEVENT hClose = ::WSACreateEvent();
        ::WSAEventSelect(s, hClose, FD_CLOSE);
        HANDLE h[2] = { hCancel, hClose };
        DWORD wait = ::WSAWaitForMultipleEvents(2, h, FALSE, WSA_INFINITE, FALSE);
        if (wait == WSA_WAIT_EVENT_0)// �˳�
        {
            ReceiveTask.Cancel();
            ::WSACloseEvent(hClose);
            break;
        }

        WSANETWORKEVENTS events = {};
        ::WSAEnumNetworkEvents(s, hClose, &events);
        int err = events.iErrorCode[FD_CLOSE_BIT];
        ::WSACloseEvent(hClose);

        ReceiveTask.Cancel();

        auto async = _Connection.get__Client()->BeginDisconnect(nullptr, NULL);
        _Connection.get__Client()->EndDisconnect(async);
    }

    return 0;
}

void ClassFactoryImpl::RemoteActivation(const RPC::RpcInvoke& invoke, ZeroCopyNetworkOutputStream& nos)
{
    RPC::VariantParam guid = invoke.params(0);
    RPC::VariantParam objectid = invoke.params(1);
    IID id = {};
    memcpy(&id, guid.guid().c_str(), sizeof(IID));

    IUnknown* p = nullptr;
    HRESULT hr = CreateInstance(id, &p);
    if (SUCCEEDED(hr))
    {
        RPC::RpcReturn ret;
    }
}

DWORD_PTR ClassFactoryImpl::OnReceive(DWORD_PTR Param, HANDLE hCancel)
{
    using namespace std;
    using namespace RPC;
    using namespace hxc;

    tcp_stream stream(_Connection.get__Client());
    ZeroCopyNetworkInputStream nis(stream, 256);
    ZeroCopyNetworkOutputStream nos(stream, 256);

    auto wait = ::WaitForSingleObject(hCancel, 0);

    while (::WaitForSingleObject(hCancel, 0) == WAIT_TIMEOUT)
    {
        try
        {
            RpcInvoke invoke;
            if (ZeroCopyNetworkInputStream::ReadDelimitedFrom(&nis, invoke))
            {
                if (invoke.objectid() == 0)
                {
                    switch (invoke.dispid())
                    {
                    case 1:
                        RemoteActivation(invoke, nos);
                        break;
                    default:
                        break;
                    }
                }
                else
                {

                }
            }
        }
        catch (const Exception&)
        {
            continue;
        }
    }

    return 0;
}
