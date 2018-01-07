#include "stdafx.h"
#include "hxc.h"
#include "ClassFactoryStub.h"

STDMETHODIMP ClassFactoryStub::CreateInstance(REFIID riid, IUnknown ** ppvObject)
{
    return E_NOTIMPL;
}

STDMETHODIMP ClassFactoryStub::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}
