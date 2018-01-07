using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using XRemote.PInvoke;
using System.Collections;
using System.Threading;
using xRemoteServerLib;
using XRemote.Common;
using RPC;
using Google.Protobuf;
using System.IO;

namespace XRemote.Network
{
    class OnHostOnlineEventArg : EventArgs
    {
        public ISystemInfo InterfaceSystemInfo { get; }

        public OnHostOnlineEventArg(ISystemInfo Interface)
        {
            InterfaceSystemInfo = Interface;
        }
    }

    class HostContext
    {
        private class InvokeContext
        {
            public InvokeContext(ManualResetEvent Event)
            {
                EventReturn = Event;
            }

            public ManualResetEvent EventReturn { get; }

            public RPC.RpcReturn Return { get; set; }
        }

        public HostContext(TcpClient conn)
        {
            Connection = conn;
            m_NetworkStream = conn.GetStream();
            CancellationTokenSource = new CancellationTokenSource();
        }

        public TcpClient Connection { get; }

        private NetworkStream m_NetworkStream { get; }

        public IRemoteClassFactory ClassFactory { get; set; }

        private Dictionary<int, InvokeContext> InvokeWaitTable { get; } = new Dictionary<int, InvokeContext>();

        private CancellationTokenSource CancellationTokenSource;

        private CancellationToken CancellationToken { get { return CancellationTokenSource.Token; } }

        public Task<RpcReturn> Invoke(RpcInvoke ri)
        {
            var invokectx = new InvokeContext(GlobalObjectPool.ManualResetEventPool.Pop());
            InvokeWaitTable.Add(ri.CallID, invokectx);

            return Task<RpcReturn>.Factory.StartNew(() =>
            {
                ri.WriteDelimitedTo(m_NetworkStream);
                m_NetworkStream.FlushAsync().Wait();

                // 调用返回时会先从InvokeWaitTable当中移除对应的InvokeContext，
                // 然后再触发InvokeContext.EventReturn，所以要使用预先保存的invokectx而不从InvokeWaitTable当中取值
                invokectx.EventReturn.WaitOne();
                GlobalObjectPool.ManualResetEventPool.Push(invokectx.EventReturn);

                return invokectx.Return;
            });
        }

        private Task ReceiveData()
        {
            return Task.Factory.StartNew(async () =>
            {
                try
                {
                    byte[] length = new byte[4];
                    int offset = 0;
                    while (offset <= 3)
                    {
                        offset += await m_NetworkStream.ReadAsync(length, offset, 4 - offset, CancellationToken);
                    }
                    var cis = new CodedInputStream(length);
                    var data = new byte[cis.ReadUInt32()];

                    offset = 4;
                    while (offset <= data.Length - 4 - 1)
                    {
                        offset += await m_NetworkStream.ReadAsync(length, offset, 4 - offset, CancellationToken);
                    }

                    using (var ms = new MemoryStream(data))
                    {
                        ms.Write(length, 0, 4);
                        OnInvokeOperationReturn(RpcReturn.Parser.ParseDelimitedFrom(ms));
                    }

                    ReceiveData();
                }
                catch (AggregateException ae)
                {
                    foreach (Exception e in ae.InnerExceptions)
                    {
                        if (e is TaskCanceledException)
                            return;
                    }
                }
                catch (Exception)
                {

                }
            });
        }

        private void OnInvokeOperationReturn(RpcReturn Return)
        {
            var invokectx = InvokeWaitTable[Return.CallID];
            invokectx.Return = Return;

            InvokeWaitTable.Remove(Return.CallID);

            invokectx.EventReturn.Set();
        }
    }

    /// <summary>
    /// 监听指定的端口，为每一个连接创建一个代理接口的实例。
    /// </summary>
    static class ProxyListener
    {
        public static event EventHandler<OnHostOnlineEventArg> OnHostOnline;

        private static TcpListener ListenSocket;

        /// <summary>
        /// Hashtable(strEndPoint, HostContext)
        /// </summary>
        private static Hashtable HostTable = Hashtable.Synchronized(new Hashtable());

        public static void Initialize()
        {
            string[] endpoint = Program.Config.Network["ListenEndPoint"].Split(':');
            string ip = endpoint[0];
            int port = int.Parse(endpoint[1]);
            var ep = new IPEndPoint(IPAddress.Parse(ip), port);
            ListenSocket = new TcpListener(ep);
            ListenSocket.Server.UseOnlyOverlappedIO = true;//仅使用重叠I/O
            ListenSocket.Start(5);
            ListenSocket.BeginAcceptTcpClient(AcceptCallback, null);
        }

        public static Dictionary<string, ISystemInfo> GetHostList()
        {
            return null;
        }

        private static void AcceptCallback(IAsyncResult ar)
        {
            var TcpClient = ListenSocket.EndAcceptTcpClient(ar);

            // Keep-Alive特性设置
            tcp_keepalive ka = new tcp_keepalive();
            ka.onoff = 1;// 启用
            ka.keepalivetime = 5 * 60 * 1000;// 无数据收发时每5min发送一次Keep-Alive包
            ka.keepaliveinterval = 10000;// Keep-Alive包发送无应答时重发间隔10s
            byte[] buff = NativeMethods.StructureToByteArray(ka);
            byte[] ret = new byte[buff.Length];
            TcpClient.Client.IOControl(IOControlCode.KeepAliveValues, buff, ret);

            //添加到新连接列表，定时遍历并关闭不发送数据的连接
            AddNewHostAsync(TcpClient);
        }

        private static void AddNewHostAsync(TcpClient conn)
        {
            if (conn == null)
                throw new ArgumentNullException();

            HostContext hc = new HostContext(conn);
            hc.ClassFactory = InterfacesImpl.ClassFactoryProxy.CreateInstance(hc); ;

            Guid iidISystemInfo = typeof(ISystemInfo).GUID;
            object Interface = null;
            try
            {
                hc.ClassFactory.CreateInstance(ref iidISystemInfo, out Interface);
                if (Interface != null)
                {
                    if (OnHostOnline != null)
                        OnHostOnline.Invoke(hc, new OnHostOnlineEventArg(Interface as ISystemInfo));
                    HostTable.Add(hc.Connection.Client.RemoteEndPoint.ToString(), hc);
                }
            }
            catch (Exception)
            {

            }
        }
    }
}
