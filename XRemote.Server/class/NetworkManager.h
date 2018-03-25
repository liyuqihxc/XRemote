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
    static HRESULT RegisterClassObject(
        _In_  REFCLSID  rclsid,
        _In_  LPUNKNOWN pUnk
    );
    static void Stop();
private:
    static DWORD_PTR EstablishConnection(DWORD_PTR Param, HANDLE hCancel);
private:
    static hxc::TcpClient _Connection;
    static std::map<int32_t, std::unique_ptr<hxc::InterfaceStub>> _ObjectMap;
    static hxc::Task _EstablishConnectionTask;
};

#endif// if !defined _NETWORKMANAGER_H_
