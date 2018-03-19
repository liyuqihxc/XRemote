#pragma once

#if !defined _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

class NetworkManager
{
public:
    NetworkManager() = delete;
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
public:
    static void Start();
    static void Stop();
private:
    static DWORD_PTR EstablishConnection(DWORD_PTR Param, HANDLE hCancel);
private:
    static hxc::TcpClient _Connection;
    static std::unique_ptr<hxc::RpcStub> _ClassFactoryStub;
    static std::map<int32_t, std::unique_ptr<hxc::RpcStub>> _ObjectMap;
    static hxc::Task _EstablishConnectionTask;
};

#endif// if !defined _NETWORKMANAGER_H_
