#pragma once

#ifndef _PROXYSTUB_H_
#define _PROXYSTUB_H_

namespace hxc
{
    typedef HRESULT (WINAPI _CREATORFUNC)(void*, REFIID, void**);

#define BEGIN_INTERFACE_MAP(x) \
protected:\
typedef hxc::ObjectCreator<x> _thisclass;\
typedef struct _Interface_Entry\
{\
    const IID* riid;\
    typedef std::function<void* (_thisclass*)> Converter;\
    Converter converter;\
} Interface_Entry;\
const static Interface_Entry* WINAPI _GetEntries()\
{\
    static const Interface_Entry entrys[]={\
    { &__uuidof(IUnknown), [](_thisclass* p) { return static_cast<IUnknown*>(p); } },

#define INTERFACE_ENTRY(x)\
{ &__uuidof(x), [](_thisclass* p) { return static_cast<x*>(p); } },

#define END_INTERFACE_MAP() \
{ nullptr, nullptr }}; return entrys;}

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
    class ObjectCreator :
        public T
    {
        explicit ObjectCreator() : _RefCount(0) {}
    public:
        ~ObjectCreator() {}

        ObjectCreator(const ObjectCreator&) = delete;
        ObjectCreator& operator=(const ObjectCreator&) = delete;

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
            _In_opt_ void* pv,
            _In_ REFIID riid,
            _COM_Outptr_ LPVOID* ppv
        )
        {
            if (ppv == NULL)
                return E_POINTER;
            *ppv = NULL;

            ObjectCreator* pret = new ObjectCreator();

            HRESULT hres = pret->ObjectCreator::FinalConstruct();
            if (FAILED(hres))
            {
                pret->Release();
                return hres;
            }

            hres = pret->QueryInterface(riid, ppv);
            if (FAILED(hres))
                pret->Release();

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

            const static T::Interface_Entry* pEntries = T::_GetEntries();
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
                ObjectCreator::FinalRelease();
                delete this;
            }
            return 0;
        }
    protected:
        volatile long _RefCount;
    };

#define DECLARE_CLASS_FACTORY(x) \
    typedef hxc::ObjectCreator<hxc::ClassFactory<hxc::ObjectCreator<x>::CreateInstance>> _ClassFactoryCreatorClass;

    template <_CREATORFUNC fn>
    class DECLSPEC_NOVTABLE ClassFactory :
        public IClassFactory
    {
    public:
        STDMETHOD(CreateInstance)(
            _Inout_opt_ LPUNKNOWN pUnkOuter,
            _In_ REFIID riid,
            _COM_Outptr_ void** ppvObj
        )
        {
            return S_OK;
        }

        STDMETHOD(LockServer)(_In_ BOOL fLock)
        {
            return S_OK;
        }

        BEGIN_INTERFACE_MAP(ClassFactory)
            INTERFACE_ENTRY(IClassFactory)
        END_INTERFACE_MAP()

        static HRESULT WINAPI CreateInstance(
            _In_opt_ void* pv,
            _In_ REFIID riid,
            _COM_Outptr_ LPVOID* ppv
        )
        {
            fn(pv, riid, ppv);
            return 0;
        }
    };

    typedef struct _Object_Entry
    {
        _CREATORFUNC *pfnGetClassObject;
        _CREATORFUNC *pfnCreateInstance;
        const CLSID* ObjGUID;
    } OBJECT_ENTRY, *POBJECT_ENTRY;

    class InterfaceStub
    {
    public:
        InterfaceStub(const InterfaceStub&) = delete;
        InterfaceStub& operator=(const InterfaceStub&) = delete;
        explicit InterfaceStub(IDispatch* pDisp);
    };

};//namespace hxc

extern hxc::OBJECT_ENTRY _OBJECT_ENTRYS[];

#endif//if !defined _PROXYSTUB_H_
