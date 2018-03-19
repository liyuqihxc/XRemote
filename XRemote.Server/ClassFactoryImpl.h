#pragma once

#if !defined _CLASSFACTORYSTUB_H_
#define _CLASSFACTORYSTUB_H_

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message_lite.h>
#include "RPC.pb.h"

class ZeroCopyNetworkInputStream : public google::protobuf::io::ZeroCopyInputStream
{
public:
    ZeroCopyNetworkInputStream(hxc::tcp_stream& tcp_stream, int32_t capacity);
    ~ZeroCopyNetworkInputStream();
    ZeroCopyNetworkInputStream(const ZeroCopyNetworkInputStream& o) = delete;
    ZeroCopyNetworkInputStream& operator=(const ZeroCopyNetworkInputStream& o) = delete;
public:
    virtual bool Next(const void** data, int* size);
    virtual void BackUp(int count);
    virtual bool Skip(int count);
    virtual long long ByteCount() const;
    static bool ReadDelimitedFrom(google::protobuf::io::ZeroCopyInputStream * rawInput, google::protobuf::MessageLite & message);
private:
    DWORD_PTR ReceiveProc(DWORD_PTR Param, HANDLE hCancel);
private:
    uint8_t * _buffer;
    const int32_t _capacity;
    volatile int32_t _head;
    volatile int32_t _ptr;

    HANDLE _signal;

    hxc::tcp_stream _tcp_stream;
    hxc::Task _recv_task;
};

class ZeroCopyNetworkOutputStream : public google::protobuf::io::ZeroCopyOutputStream
{
public:
    ZeroCopyNetworkOutputStream(hxc::tcp_stream& tcp_stream, int32_t capacity);
    ~ZeroCopyNetworkOutputStream();
    ZeroCopyNetworkOutputStream(const ZeroCopyNetworkOutputStream& o) = delete;
    ZeroCopyNetworkOutputStream& operator=(const ZeroCopyNetworkOutputStream& o) = delete;
public:
    virtual bool Next(void** data, int* size);
    virtual void BackUp(int count);
    virtual long long ByteCount() const;
    virtual bool WriteAliasedRaw(const void* data, int size);
    virtual bool AllowsAliasing() const { return true; }
    static bool WriteDelimitedTo(const google::protobuf::MessageLite & message, google::protobuf::io::ZeroCopyOutputStream * rawOutput);
private:
    hxc::tcp_stream _tcp_stream;

    uint8_t * _buffer;
    const int32_t _capacity;
    int32_t _ptr;

    bool _allow_backup;
};

class ClassFactoryImpl :
    public IRemoteClassFactory
{
public:
    ClassFactoryImpl();

    STDMETHOD(CreateInstance)(
        _In_ REFIID riid,
        _COM_Outptr_  IUnknown **ppvObject
        );

    STDMETHOD(LockServer)(BOOL fLock);

    HRESULT FinalConstruct();
    void FinalRelease();
private:
    DWORD_PTR OnReceive(DWORD_PTR Param, HANDLE hCancel);
    DWORD_PTR MaintainConnection(DWORD_PTR Param, HANDLE hCancel);
    void RemoteActivation(const RPC::RpcInvoke& invoke, ZeroCopyNetworkOutputStream& nos);
protected:
    typedef hxc::ObjectCreator<ClassFactoryImpl> _thisclass;
    typedef struct _Interface_Entry
    {
        const IID* riid;
        typedef std::function<void* (_thisclass*)> Converter;
        Converter converter;
    } Interface_Entry;
    const static Interface_Entry* WINAPI _GetEntries()
    {
        static const Interface_Entry entrys[] =
        {
            { &__uuidof(IUnknown), [](_thisclass* p) { return static_cast<IUnknown*>(p); } },
            { nullptr, nullptr }
        };
        return entrys;
    }

private:
    std::wstring _RemoteIP;
    uint16_t _RemotePort;
    hxc::TcpClient _Connection;
    hxc::Task _MaintainConnectionTask;
    std::map<int32_t, IDispatch*> _interface_map;
};

#endif// if !defined _CLASSFACTORYSTUB_H_
