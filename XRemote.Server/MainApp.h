#pragma once

#include "Res.h"
#include "ClassFactoryImpl.h"

class MainApp : public hxc::ServiceBase
{
public:
    DECLARE_SERVICE_NAME(L"xremote")

    inline static ServiceBase* WINAPI CreateNew()
    {
        return static_cast<ServiceBase*>(new MainApp());
    }

    virtual void OnStart();
    virtual void OnStop();

private:
    std::unique_ptr<hxc::rpc::CStub<ClassFactoryImpl>> _ClassFactoryStub;
};
