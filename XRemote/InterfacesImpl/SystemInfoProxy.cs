using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XRemote.Network;
using xRemoteServerLib;

namespace XRemote.InterfacesImpl
{
    class SystemInfoProxy :
        Proxy<SystemInfoProxy>,
        ISystemInfo
    {
        public SystemInfoProxy()
        {

        }
    }
}
