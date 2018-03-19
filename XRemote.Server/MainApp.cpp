// XRemote.Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "hxc.h"
#define X264_API_IMPORTS
#include "../../libx264/include/x264.h"
#include "MainApp.h"
#include <iostream>

#pragma comment(lib, "Rpcrt4.lib")


hxc::SERVICE_MAP_ENTRYW map[] =
{
    { L"xremote", &MainApp::CreateNew },
    { nullptr, nullptr}
};
hxc::ServiceHost::ServiceHost()
{
    this->m_pMap = map;
}
hxc::ServiceHost __host_;

void MainApp::OnStart()
{
    for (int i = 0; i < 15; i++)
    {
        if (IsDebuggerPresent())
            break;
        Sleep(1000);
    }
    hxc::_DataPool::Initialize();

}

void MainApp::OnStop()
{
    Sleep(1000);
    hxc::_DataPool::Free();
}

int _tmain(int argc, char *argv[])
{
    hxc::_DataPool::Initialize();

    hxc::ObjectCreator<ClassFactoryImpl> * p = nullptr;
    hxc::ObjectCreator<ClassFactoryImpl>::CreateInstance(reinterpret_cast<void**>(&p));
    std::unique_ptr<hxc::ObjectCreator<ClassFactoryImpl>> ptr(p);

    char a[10];
    std::cin >> a;
    hxc::_DataPool::Free();
    return 0;
}
