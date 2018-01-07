#pragma once
#include "ClassFactoryStub.h"
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
    static hxc::CStub<ClassFactoryStub>* _ClassFactoryStub;
    static hxc::Task* _EstablishConnectionTask;
    static HANDLE _EventStop;
};

#endif// if !defined _NETWORKMANAGER_H_
