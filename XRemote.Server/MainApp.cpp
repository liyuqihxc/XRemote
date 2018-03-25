// XRemote.Server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "xRemoteServer_h.h"
#include "hxc.h"
#define X264_API_IMPORTS
#include "MainApp.h"
#include <iostream>
#include "FileSystemImpl.h"

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

hxc::OBJECT_ENTRY _OBJECT_ENTRYS[] = 
{
    { FileSystemImpl::_ClassFactoryCreatorClass::CreateInstance, hxc::ObjectCreator<FileSystemImpl>::CreateInstance, &__uuidof(XRemoteServerFileSystem) },
    { nullptr, nullptr, nullptr }
};

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
    //hxc::ObjectCreator<ClassFactoryImpl>::CreateInstance(reinterpret_cast<void**>(&p));
    std::unique_ptr<hxc::ObjectCreator<ClassFactoryImpl>> ptr(p);

    char a[10];
    std::cin >> a;
    hxc::_DataPool::Free();
    return 0;
}
