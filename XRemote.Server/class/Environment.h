#pragma once
#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

namespace hxc
{
    class Environment
    {
    private:
        Environment() {}
    public:
        static DWORD __stdcall get__ProcessorCount();
    private:
        static DWORD _ProcessorCount;
    };
} // namespace hxc

#endif // ! _ENVIRONMENT_H_
