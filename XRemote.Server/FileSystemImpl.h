#pragma once
#ifndef _FILESYSTEM_IMPL_H_
#define _FILESYSTEM_IMPL_H_

class DECLSPEC_NOVTABLE FileSystemImpl :
    public hxc::IDispatchImpl<IFileSystem>
{
public:
    DECLARE_CLASS_FACTORY(FileSystemImpl)

    BEGIN_INTERFACE_MAP(FileSystemImpl)
        INTERFACE_ENTRY(IFileSystem)
    END_INTERFACE_MAP()

public:
    STDMETHOD(GetFolder)(LPCOLESTR lpPath, IFolder** pp);
    STDMETHOD(GetFile)(LPCOLESTR lpPath, IFile** pp);
    STDMETHOD(GetDrive)(LPCOLESTR lpPath, IDrive** pp);
};

#endif // !defined _FILESYSTEM_IMPL_H_