using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Reflection;
using XRemote.PInvoke;
using System.Collections;
using Google.Protobuf.WellKnownTypes;
using Google.Protobuf;
using System.IO;
using System.Threading;
using RPC;
using XRemote.Common;
using XRemote.Network;

namespace XRemote.InterfacesImpl
{
    class Proxy<T> where T : class, new()
    {
        /// <summary>
        /// Hashtable(CallTask.CallID, CallTask)
        /// </summary>
        private Hashtable m_CallMap = Hashtable.Synchronized(new Hashtable());

        protected int ObjectID { get; set; }

        protected HostContext m_HostContext { get; private set; }

        /// <summary>
        /// 创建并初始化代理对象
        /// </summary>
        /// <param name="hc">HostContext</param>
        /// <returns></returns>
        public static T CreateInstance(HostContext hc)
        {
            if (hc == null)
                throw new ArgumentNullException();

            T t =  new T();
            var proxy = t as Proxy<T>;
            proxy.ObjectID = proxy.GetHashCode();
            proxy.m_HostContext = hc;

            return t;
        }

        /// <summary>
        /// 查找指定方法实现的接口成员并调用。
        /// </summary>
        /// <typeparam name="I">实现的接口</typeparam>
        /// <param name="mb">需要调用的方法</param>
        /// <param name="Params">参数</param>
        protected Task<RpcReturn> InvokeRemoteMemberAsync<I>(MethodBase mb, object[] Params)
        {
            if (mb == null)
                throw new ArgumentNullException("mb");

            // 取得指定方法实现的接口方法
            var im = GetType().GetInterfaceMap(typeof(I));
            int index = Array.FindIndex(im.TargetMethods, (m) =>
            {
                return m == (mb as MethodInfo);
            });

            if (index == -1)
                throw new ArgumentException($"接口 {typeof(I).Name} 未定义成员 {mb.Name}。", "mb");

            var BaseMethod = im.InterfaceMethods[index];
            // 验证参数是否对应
            /*var BaseMethod_Params = BaseMethod.GetParameters().Select(p => p.ParameterType).ToArray();
            if (BaseMethod_Params.Length != Params.Length)// 验证参数数量是否对应
                throw new ArgumentException("提供的参数数量无法与指定接口的成员对应。");

            for (int i = 0; i != BaseMethod_Params.Length; i++)// 依次验证传入参数与接口方法参数类型是否对应
            {
                if (Params[i] == null)
                    continue;

                System.Type ParamType = Params[i].GetType();

                if (BaseMethod_Params[i].IsByRef && BaseMethod_Params[i].GetElementType() != ParamType)
                {
                    // ref、out 类型的参数
                    throw new ArgumentException();
                }
                if (BaseMethod_Params[i].IsArray && (!ParamType.IsArray || BaseMethod_Params[i].GetElementType() != ParamType))
                {
                    // 数组
                    throw new ArgumentException();
                }
                else if (BaseMethod_Params[i] != Params[i].GetType())
                {
                    throw new ArgumentException();
                }
            }*/

            // 获取 DispID 和 DispInvokeFlag
            DispIdAttribute dia = Attribute.GetCustomAttribute(BaseMethod, typeof(DispIdAttribute)) as DispIdAttribute;
            if (dia == null)
                throw new ArgumentException($"{mb.Name} 不包含 DispIdAttribute 特性。", "mb");

            int DispID = dia.Value;
            DispInvokeFlag wFlags = 0;
            switch (BaseMethod.MemberType)
            {
                case MemberTypes.Method:
                    wFlags = DispInvokeFlag.DISPATCH_METHOD;
                    break;
                case MemberTypes.Property:
                    break;
                case MemberTypes.Event:
                    break;
                default:
                    throw new ArgumentException("不支持的成员类型。");
            }

            var invoke = new RPC.RpcInvoke();
            invoke.InterfaceID = Crc32.Hash(typeof(I).GUID.ToByteArray(), 0, 16);
            invoke.CallID = invoke.GetHashCode();
            invoke.ObjectID = ObjectID;
            invoke.DispID = DispID;
            invoke.WFlags = (ushort)wFlags;
            foreach (var p in Params)
            {
                var vp = new VariantParam();

                if (p == null)
                {

                }
                else if (p is int || p is sbyte || p is short)
                    vp.SFixed32 = (int)p;
                else if (p is uint || p is byte || p is ushort)
                    vp.Fixed32 = (uint)p;
                else if (p is long)
                    vp.SFixed64 = (long)p;
                else if (p is ulong)
                    vp.Fixed64 = (ulong)p;
                else if (p is bool)
                    vp.Bool = (bool)p;
                else if (p is float)
                    vp.Float = (float)p;
                else if (p is double)
                    vp.Double = (double)p;
                else if (p is string)
                    vp.String = p as string;
                else if (p is decimal)
                    vp.Decimal = p.ToString();
                else if (p is byte[])
                    vp.Bytes = ByteString.CopyFrom(p as byte[]);
                else if (p is Guid)
                    vp.Guid = ByteString.CopyFrom(((Guid)p).ToByteArray());
                else if (p.GetType().GetInterfaces().FirstOrDefault(t => t == typeof(IMessage)) != null)
                {

                }
                else
                    throw new ArgumentException();

                invoke.Params.Add(vp);
            }

            return m_HostContext.Invoke(invoke);
        }
    }
}
