#pragma once

#if !defined _CLASSFACTORYSTUB_H_
#define _CLASSFACTORYSTUB_H_

class ClassFactoryStub :
    public hxc::IDispatchImpl<IRemoteClassFactory>
{
public:
    STDMETHOD(CreateInstance)(
        _In_ REFIID riid,
        _COM_Outptr_  IUnknown **ppvObject
        );
    STDMETHOD(LockServer)(BOOL fLock);
protected:
    typedef hxc::CStub<ClassFactoryStub> _thisclass;
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
            { &__uuidof(IDispatch), [](_thisclass* p) {return static_cast<IDispatch*>(p); } },
            { nullptr, nullptr }
        };
        return entrys;
    }
};

#endif// if !defined _CLASSFACTORYSTUB_H_
