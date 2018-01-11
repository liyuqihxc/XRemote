// XRemote.Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdint.h>
#define X264_API_IMPORTS
#include "../../libx264/include/x264.h"
#include "hxc.h"
#include "MainApp.h"

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
    using namespace hxc;

    for (int i = 0; i < 15; i++)
    {
        if (IsDebuggerPresent())
            break;
        Sleep(1000);
    }

    WSADATA wd;
    hxc::Socket::InitializeWinsock(MAKEWORD(2, 2), &wd);

    //hxc::ThreadPool::Start();
    hxc::_DataPool::Initialize();

}

void MainApp::OnStop()
{
    Sleep(1000);
    hxc::_DataPool::Free();
    //hxc::ThreadPool::Stop();

    hxc::Socket::UninitializeWinsock();
}

int _tmain(int argc, char *argv[])
{
    hxc::_DataPool::Initialize();

    NetworkManager::Start();

    Sleep(1000);
    hxc::_DataPool::Free();
    return 0;
}
