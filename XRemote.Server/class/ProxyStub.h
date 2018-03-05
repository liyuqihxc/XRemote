﻿#pragma once

#ifndef _BACKDOOR_PROXYSTUB_H_
#define _BACKDOOR_PROXYSTUB_H_

#include <mstcpip.h>
#include "xRemoteServer_h.h"

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

    template<class T>
    class CStub :
        public T
    {
        explicit CStub() : _RefCount(0) {}
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
            __if_exists(T::FinalRelease)
            {
                T::FinalRelease();
            }
        }

        static HRESULT WINAPI CreateInstance(
            _COM_Outptr_ void** ppv
        )
        {
            if (ppv == NULL)
                return E_POINTER;
            *ppv = NULL;

            CStub* pret = new CStub();

            HRESULT hres = pret->CStub::FinalConstruct();
            if (FAILED(hres))
            {
                pret->Release();
                return hres;
            }
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
        volatile long _RefCount;
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
