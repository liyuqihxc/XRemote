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
    static DWORD_PTR EstablishConnection(DWORD_PTR Param);
private:
    static hxc::TcpClient _Connection;
    static std::unique_ptr<hxc::CStub<ClassFactoryImpl>> _ClassFactoryStub;
    static std::unique_ptr<hxc::Task> _EstablishConnectionTask;
    static HANDLE _EventStop;
};

#endif// if !defined _NETWORKMANAGER_H_
