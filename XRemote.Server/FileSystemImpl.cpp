#include "stdafx.h"
#include "xRemoteServer_h.h"
#include "hxc.h"
#include "FileSystemImpl.h"

STDMETHODIMP FileSystemImpl::GetFolder(LPCOLESTR lpPath, IFolder ** pp)
{
    return E_NOTIMPL;
}

STDMETHODIMP FileSystemImpl::GetFile(LPCOLESTR lpPath, IFile ** pp)
{
    return E_NOTIMPL;
}

STDMETHODIMP FileSystemImpl::GetDrive(LPCOLESTR lpPath, IDrive ** pp)
{
    return E_NOTIMPL;
}
