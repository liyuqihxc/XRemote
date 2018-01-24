#pragma once

#include "ClassFactoryImpl.h"

#if !defined _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

class NetworkManager
{
private:
    NetworkManager() {}
    NetworkManager(const NetworkManager&) {}
public:
    static void Start();
    static void Stop();
private:
    static DWORD_PTR EstablishConnection(DWORD_PTR Param, HANDLE hCancel);
private:
    static hxc::TcpClient _Connection;
    static std::unique_ptr<hxc::rpc::CStub<ClassFactoryImpl>> _ClassFactoryStub;
    static hxc::Task _EstablishConnectionTask;
};

#endif// if !defined _NETWORKMANAGER_H_
