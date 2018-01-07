#include "stdafx.h"
#include "hxc.h"

void hxc::_DataPool::Initialize()
{
    using namespace hxc;

    sm_pManualResetEventPool = new ObjectPool<HANDLE>(
        10,
        10,
        []() { return ::CreateEventW(nullptr, true, false, nullptr); },
        [](HANDLE& h) { ::ResetEvent(h); },
        [](HANDLE& h) { ::CloseHandle(h); }
    );

    sm_pTCPSocketPool = new ObjectPool<SOCKET>(
        10,
        10,
        []() { return ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED); },
        nullptr,
        [](SOCKET s) { ::closesocket(s); }
    );

    sm_pBufferPool = new ObjectPool<BYTE*>(
        10,
        10,
        []() { return (BYTE*)malloc(BufferSize); },
        [](BYTE*& p) { ZeroMemory(p, BufferSize); },
        [](BYTE*& p) { free(p); }
    );
}

void hxc::_DataPool::Free()
{
    delete sm_pManualResetEventPool;
    delete sm_pTCPSocketPool;
    delete sm_pBufferPool;
}

hxc::ObjectPool<HANDLE>* hxc::_DataPool::sm_pManualResetEventPool;
hxc::ObjectPool<SOCKET>* hxc::_DataPool::sm_pTCPSocketPool;
hxc::ObjectPool<BYTE*>* hxc::_DataPool::sm_pBufferPool;

