#pragma once

#ifndef _BACKDOOR_PROXYSTUB_H_
#define _BACKDOOR_PROXYSTUB_H_

#include <mstcpip.h>
//#include "Pool.h"
#include "xRemoteServer_h.h"
#include "RPC.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace hxc
{
namespace rpc
{
    class _TypeInfoHolder
    {
    public:
        const GUID* m_pguid;
        //const GUID* m_plibid;

        ITypeInfo* m_pInfo;
        struct stringdispid
        {
            BSTR bstr;
            int nLen;
            DISPID id;
            stringdispid() : nLen(0), id(DISPID_UNKNOWN), bstr(nullptr) {}
            ~stringdispid()
            {
                if (bstr)
                    SysFreeString(bstr);
            }
        };
        stringdispid* m_pMap;
        int m_nCount;
    public:
        HRESULT GetTI(
            _In_ LCID lcid,
            _Outptr_result_maybenull_ ITypeInfo** ppInfo);

        HRESULT GetTI(_In_ LCID lcid);
        HRESULT EnsureTI(_In_ LCID lcid);

        // This function is called by the module on exit
        void Cleanup();

        HRESULT GetTypeInfo(
            _In_ UINT itinfo,
            _In_ LCID lcid,
            _Outptr_result_maybenull_ ITypeInfo** pptinfo);

        HRESULT GetIDsOfNames(
            _In_ REFIID /* riid */,
            _In_reads_(cNames) LPOLESTR* rgszNames,
            _In_range_(0, 16384) UINT cNames,
            LCID lcid,
            _Out_ DISPID* rgdispid);

        _Check_return_ HRESULT Invoke(
            _Inout_ IDispatch* p,
            _In_ DISPID dispidMember,
            _In_ REFIID /* riid */,
            _In_ LCID lcid,
            _In_ WORD wFlags,
            _In_ DISPPARAMS* pdispparams,
            _Out_opt_ VARIANT* pvarResult,
            _Out_opt_ EXCEPINFO* pexcepinfo,
            _Out_opt_ UINT* puArgErr);

        _Check_return_ HRESULT LoadNameCache(_Inout_ ITypeInfo* pTypeInfo);
    };

    template<class T, const IID* piid = &__uuidof(T), 
        class tihclass = _TypeInfoHolder>
    class DECLSPEC_NOVTABLE IDispatchImpl : public T
    {
    public:
        typedef tihclass _tihclass;
    public:
        STDMETHOD(GetTypeInfoCount)(_Out_ UINT* pctinfo)
        {
            if (pctinfo == NULL)
                return E_POINTER;
            *pctinfo = 1;
            return S_OK;
        }
        STDMETHOD(GetTypeInfo)(
            UINT itinfo,
            LCID lcid,
            _Outptr_result_maybenull_ ITypeInfo** pptinfo)
        {
            return _tih.GetTypeInfo(itinfo, lcid, pptinfo);
        }
        STDMETHOD(GetIDsOfNames)(
            _In_ REFIID riid,
            _In_reads_(cNames) _Deref_pre_z_ LPOLESTR* rgszNames,
            _In_range_(0, 16384) UINT cNames,
            LCID lcid,
            _Out_ DISPID* rgdispid)
        {
            return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
        }
        STDMETHOD(Invoke)(
            _In_ DISPID dispidMember,
            _In_ REFIID riid,
            _In_ LCID lcid,
            _In_ WORD wFlags,
            _In_ DISPPARAMS* pdispparams,
            _Out_opt_ VARIANT* pvarResult,
            _Out_opt_ EXCEPINFO* pexcepinfo,
            _Out_opt_ UINT* puArgErr)
        {
            return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        }
    protected:
        static _tihclass _tih;
        static HRESULT GetTI(
            _In_ LCID lcid,
            _Outptr_ ITypeInfo** ppInfo)
        {
            return _tih.GetTI(lcid, ppInfo);
        }
    };

    template <class T, const IID* piid, class tihclass>
    typename IDispatchImpl<T, piid, tihclass>::_tihclass
        IDispatchImpl<T, piid, tihclass>::_tih =
    { piid, NULL, NULL, 0 };

    class ZeroCopyNetworkInputStream
    {
    public:
    	ZeroCopyNetworkInputStream(tcp_stream& tcp_stream, int capacity);
    	~ZeroCopyNetworkInputStream();
        ZeroCopyNetworkInputStream(const ZeroCopyNetworkInputStream& o) = delete;
        ZeroCopyNetworkInputStream& operator=(const ZeroCopyNetworkInputStream& o) = delete;
    public:
        virtual bool Next(const void** data, int* size);
        virtual void BackUp(int count);
        virtual bool Skip(int count);
        virtual long long ByteCount() const;
    private:
        DWORD_PTR ReceiveProc(DWORD_PTR Param, HANDLE hCancel);
    private:
        uint8_t* _buffer;
        const int32_t _capacity;
        volatile int32_t _head;
        volatile int32_t _ptr;

        HANDLE _signal;

        tcp_stream _tcp_stream;
        Task _recv_task;
    };

    class LengthDelimitedNetworkInputStream : public google::protobuf::io::ZeroCopyInputStream
    {
    private:
        LengthDelimitedNetworkInputStream(const LengthDelimitedNetworkInputStream& o) = delete;
        LengthDelimitedNetworkInputStream& operator=(const LengthDelimitedNetworkInputStream& o) = delete;
    public:
        explicit LengthDelimitedNetworkInputStream(tcp_stream& netstream, LPBYTE pBuffer, int size);
        virtual ~LengthDelimitedNetworkInputStream();
    public:
        virtual bool Next(const void** data, int* size);
        virtual void BackUp(int count);
        virtual bool Skip(int count);
        virtual long long ByteCount() const;
    public:
        static bool ReadDelimitedFrom(
            google::protobuf::io::ZeroCopyInputStream* rawInput,
            google::protobuf::MessageLite& message
        );
    private:
        DWORD_PTR ReceiveProc(DWORD_PTR Param, HANDLE hCancel);
    private:
        tcp_stream _stream;
        Task _recv_task;

        HANDLE _signal;

        uint64_t _byte_count;

        uint32_t _buffer_size;
        uint32_t _current_size;
        uint32_t _remain_size;
        LPBYTE _direct_buffer;
    };

    class LengthDelimitedNetworkOutputStream : public google::protobuf::io::ZeroCopyOutputStream
    {
    private:
        LengthDelimitedNetworkOutputStream(const LengthDelimitedNetworkOutputStream& o) {}
        LengthDelimitedNetworkOutputStream& operator=(const LengthDelimitedNetworkOutputStream& o) {}
    public:
        explicit LengthDelimitedNetworkOutputStream(tcp_stream& netstream);
        virtual ~LengthDelimitedNetworkOutputStream();
    public:
        virtual bool Next(void** data, int* size);
        virtual void BackUp(int count);
        virtual long long ByteCount() const;
        virtual bool WriteAliasedRaw(const void* data, int size);
        virtual bool AllowsAliasing() const;
    public:
        static bool WriteDelimitedTo(
            const google::protobuf::MessageLite& message,
            google::protobuf::io::ZeroCopyOutputStream* rawOutput
        );
    };

    template<class T>
    class CStub :
        public T
    {
        CStub() = delete;
        explicit CStub(TcpClient& socket) : _ControlSock(socket), _RefCount(0), _EventStop(hxc::_DataPool::ManualResetEventPool().Pop()),
            _ReceiveTask(Task::BindFunction(&CStub::OnReceive, this), NULL, WT_EXECUTEINLONGTHREAD)
        {
        }
    public:
        ~CStub()
        {
            CStub::FinalRelease();
        }

        CStub(const CStub&) = delete;
        CStub& operator=(const CStub&) = delete;

        HRESULT FinalConstruct()
        {
            HRESULT hr = S_OK;
            __if_exists(T::FinalConstruct)
            {
                hr = T::FinalConstruct();
            }
            return hr;
        }

        void FinalRelease()
        {
            //if (_ControlSock != nullptr)
                //delete _ControlSock;
            ::SetEvent(_EventStop);
            hxc::_DataPool::ManualResetEventPool().Push(_EventStop);

            __if_exists(T::FinalRelease)
            {
                T::FinalRelease();
            }
        }

        static HRESULT WINAPI CreateInstance(
            _In_ TcpClient& tcpClient,
            _COM_Outptr_ void** ppv
        )
        {
            if (ppv == NULL)
                return E_POINTER;
            *ppv = NULL;

            CStub* pret = new CStub(tcpClient);

            HRESULT hres = pret->CStub::FinalConstruct();
            if (FAILED(hres))
            {
                pret->Release();
                return hres;
            }
            pret->_ReceiveTask.Start();
            *ppv = pret;

            pret->AddRef();//因为引用计数初始值为0
            return hres;
        }
    public:
        STDMETHOD(QueryInterface)(REFIID riid, _COM_Outptr_ void **ppvObject)
        {
            if (*ppvObject == nullptr)
            {
                //Release();
                return E_POINTER;
            }
            *ppvObject = nullptr;

            const static Interface_Entry* pEntries = T::_GetEntries();
            for (;pEntries->riid != nullptr; pEntries++)
            {
                if (IsEqualGUID(riid, *pEntries->riid))
                {
                    *ppvObject = (IUnknown*)pEntries->converter(this);
                    AddRef();
                    return S_OK;
                }
            }

            //Release();
            return E_NOINTERFACE;
        }

        STDMETHOD_(ULONG, AddRef)(void)
        {
            return _InterlockedIncrement(&_RefCount);
        }

        STDMETHOD_(ULONG, Release)(void)
        {
            if (_InterlockedDecrement(&_RefCount) <= 0)
            {
                delete this;
            }
            return 0;
        }
    protected:
        DWORD_PTR OnReceive(DWORD_PTR Param, HANDLE hCancel)
        {
            using namespace std;
            using namespace RPC;

            LPBYTE pBuffer = _DataPool::BufferPool().Pop();
            tcp_stream stream(_ControlSock.get__Client());
            LengthDelimitedNetworkInputStream nis(stream, pBuffer, _DataPool::BufferSize);

            while (::WaitForSingleObject(_EventStop, 0) == WAIT_TIMEOUT)
            {
                try
                {
                    RpcInvoke invoke;
                    if (LengthDelimitedNetworkInputStream::ReadDelimitedFrom(&nis, invoke))
                    {
                        
                    }
                }
                catch (const Exception&)
                {
                    break;
                }
            }

            _DataPool::BufferPool().Push(pBuffer);

            return 0;
        }

        void ProcessData(void)
        {
        }

    protected:
        std::wstring _RemoteIP;
        USHORT _RemotePort;
        volatile long _RefCount;
        TcpClient& _ControlSock;
        Task _ReceiveTask;
        HANDLE _EventStop;
    };

    typedef struct _Object_Entry
    {
        typedef HRESULT(WINAPI _CREATORFUNC)(
            _In_opt_ void* pv,
            _In_ REFIID riid,
            _COM_Outptr_ LPVOID* ppv
            );

        _CREATORFUNC *pfnCreateInstance;
        const GUID& ObjGUID;
    } OBJECT_ENTRY, *POBJECT_ENTRY;

};//namespace rpc
};//namespace hxc

#endif//if !defined _BACKDOOR_PROXYSTUB_H_
