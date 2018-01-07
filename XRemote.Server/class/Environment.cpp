#include "stdafx.h"
#include "hxc.h"

namespace hxc
{
    DWORD Environment::_ProcessorCount = -1;
    DWORD __stdcall Environment::get__ProcessorCount()
    {
        if (_ProcessorCount == -1)
        {
            SYSTEM_INFO si = {};
            GetSystemInfo(&si);
            _ProcessorCount = si.dwNumberOfProcessors;
        }
        return _ProcessorCount;
    }
}
