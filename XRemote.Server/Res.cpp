#include "stdafx.h"
#include "Resource.h"
#include "hxc.h"
#include "Res.h"

extern "C" const IMAGE_DOS_HEADER __ImageBase;

std::wstring Resource::get__Password(void)
{
    int i = 0;
    LPCWSTR pstr = InternalLoadString(IDS_PASSWORD, &i);
    return std::wstring(pstr, i);
}

std::wstring Resource::get__RemoteAddress(void)
{
    int i = 0;
    LPCWSTR pstr = InternalLoadString(IDS_REMOTEADDRESS, &i);
    return std::wstring(pstr, i);
}

USHORT Resource::get__RemotePort(void)
{
    int i = 0;
    LPCWSTR pstr = InternalLoadString(IDS_REMOTEPORT, &i);
    return static_cast<unsigned short>(std::stoi(std::wstring(pstr, i)));
}

DWORD Resource::get__RpcMessageVersion(void)
{
    int i = 0;
    LPCWSTR pstr = InternalLoadString(IDS_RPCMESSAGEVERSION_HIGH, &i);
    int high = static_cast<unsigned short>(std::stoi(std::wstring(pstr, i)));

    pstr = InternalLoadString(IDS_RPCMESSAGEVERSION_LOW, &i);
    int low = static_cast<unsigned short>(std::stoi(std::wstring(pstr, i)));
    return (DWORD)MAKELONG(low, high);
}

LPCWSTR Resource::InternalLoadString(int ID, int* pCount)
{
    LPCWSTR pstr = nullptr;
    *pCount = LoadStringW(GetModuleHandleW(nullptr), ID, (LPWSTR)&pstr, 0);
    return pstr;
}

