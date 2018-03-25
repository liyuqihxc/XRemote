using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using XRemote.Controlor.PInvoke;
using xRemoteServerLib;
using RPC;
using XRemote.Controlor.Network;

namespace XRemote.Controlor.InterfacesImpl
{
    class ClassFactoryProxy :
        Proxy<ClassFactoryProxy>,
        IRemoteClassFactory
    {
        public ClassFactoryProxy()
        {
            
        }

        /// <summary>
        /// 类工厂的ObjectID永远为0
        /// </summary>
        /// <param name="hc">HostContext</param>
        /// <returns></returns>
        public static new ClassFactoryProxy CreateInstance(HostContext hc)
        {
            var proxy = Proxy<ClassFactoryProxy>.CreateInstance(hc);
            proxy.ObjectID = 0;
            return proxy;
        }

        public void CreateInstance(ref Guid riid, out object ppv)
        {
            Guid iid = riid;
            object result = null;
            if (iid == typeof(ISystemInfo).GUID)
            {
                result = SystemInfoProxy.CreateInstance(m_HostContext) as ISystemInfo;
            }
            else
            {
                System.Runtime.InteropServices.Marshal.ThrowExceptionForHR((int)HRESULT.E_NOINTERFACE);
            }

            var Params = new object[] { riid, result.GetHashCode() };
            var t = InvokeRemoteMemberAsync<IRemoteClassFactory>(
                MethodBase.GetCurrentMethod(),
                Params
            );

            t.Wait();
            var ret = t.Result;

            ppv = result;
        }

        public void LockServer(int fLock)
        {
            throw new NotImplementedException();
        }
    }
}
