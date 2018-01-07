#pragma once
#ifndef _RESOURCE_H_
#define _RESOURCE_H_

class Resource
{
private:
    Resource() {}
public:
    static std::wstring get__Password(void);
    static std::wstring get__RemoteAddress(void);
    static USHORT get__RemotePort(void);
    static DWORD get__RpcMessageVersion(void);
private:
    static LPCWSTR InternalLoadString(int ID, int* pCount);
};

#endif // ifndef _RESOURCE_H_
