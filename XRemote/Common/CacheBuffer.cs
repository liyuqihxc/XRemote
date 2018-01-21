using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using XRemote.InterfacesImpl;

namespace XRemote.Common
{
    class QueueBuffer
    {
        static QueueBuffer()
        {
            for (int i = 0; i != 20; i++)
                FreeBufferPool.Push(new byte[BufferElementSize]);
        }

        private static Stack<byte[]> FreeBufferPool = new Stack<byte[]>();

        public const int BufferElementSize = 100;

        public QueueBuffer()
        {
            GetFreeBuffer();
        }

        private Queue<byte[]> BufferFilled = new Queue<byte[]>();

        /// <summary>
        /// BufferFilled第一块缓冲区的当前索引，相当于QueueBuffer的头指针
        /// </summary>
        private int HeadBufferIndex;

        private int TailBufferIndex;

        private object Lock = new object();

        public int Length { get; private set; }

        public void Write(byte[] buffer, int offset, int count)
        {
            lock (Lock)
            {
                int o = offset, c = count;
                
                while (c != 0)
                {
                    int TailBufferRemain = BufferElementSize - TailBufferIndex;
                    
                    if (TailBufferRemain > c)
                    {
                        Buffer.BlockCopy(buffer, o, BufferFilled.Last(), TailBufferIndex, c);
                        TailBufferIndex += c;
                        break;
                    }
                    else
                    {
                        //拷贝尾缓冲区剩余空闲数量
                        Buffer.BlockCopy(buffer, o, BufferFilled.Last(), TailBufferIndex, TailBufferRemain);
                        o += TailBufferRemain;
                        c -= TailBufferRemain;

                        GetFreeBuffer();
                    }
                }

                Length += count;
            }
        }

        public int Read(byte[] buffer, int offset, int count)
        {
            lock (Lock)
            {
                int c = count, o = offset, total = 0;
                while (c != 0 && BufferFilled.Count >= 1)
                {
                    if (BufferFilled.Count > 1)
                    {
                        int HeadRemain = BufferElementSize - HeadBufferIndex;
                        if (HeadRemain > c)// BufferFilled第一块缓冲区中的剩余数据数量大于需求量
                        {
                            //直接拷贝数据、修改HeadBufferIndex，然后返回
                            Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, buffer, o, c);
                            HeadBufferIndex += c;
                            total += c;
                            break;
                        }
                        else
                        {
                            Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, buffer, o, HeadRemain);
                            c -= HeadRemain;
                            o += HeadRemain;
                            total += HeadRemain;

                            RevertBuffer();
                        }
                    }
                    else
                    {
                        int Remain = TailBufferIndex - HeadBufferIndex;
                        if (Remain == 0)// 没有数据可读
                            break;

                        if (Remain > c)
                        {
                            Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, buffer, o, c);
                            HeadBufferIndex += c;
                            total += c;
                            break;
                        }
                        else
                        {
                            Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, buffer, o, Remain);
                            c -= Remain;
                            o += Remain;
                            total += Remain;

                            RevertBuffer();
                        }
                    }
                }

                Length -= total;
                return total;
            }
        }

        /// <summary>
        /// 缓冲区的头4个字节总是一个数据包的长度，所以这里只读取头四个字节将其转为一个int
        /// </summary>
        /// <returns></returns>
        public bool PeekHeader(out int DataLength)
        {
            lock (Lock)
            {
                byte[] ByteInt = new byte[4];

                if (BufferFilled.Count > 1)
                {
                    int HeadRemain = BufferElementSize - HeadBufferIndex;
                    if (HeadRemain > 4)// BufferFilled第一块缓冲区中的剩余数据数量大于需求量
                    {
                        //直接拷贝数据
                        Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, ByteInt, 0, 4);
                    }
                    else
                    {
                        if (BufferFilled.Count == 2 && TailBufferIndex < (4 - HeadRemain))// 只有2块缓冲区，验证总数据量是否大于4字节
                        {
                            DataLength = -1;
                            return false;
                        }

                        Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, ByteInt, 0, HeadRemain);

                        if (HeadRemain < 4)// 第一块缓冲区当中没有包括所有的4字节数据，再从第二块缓冲区复制
                            Buffer.BlockCopy(BufferFilled.ElementAt(1), 0, ByteInt, HeadRemain, 4 - HeadRemain);
                    }
                }
                else if (TailBufferIndex - HeadBufferIndex >= 4)// 只有1块缓冲区，验证数据量是否大于4字节
                {
                    Buffer.BlockCopy(BufferFilled.First(), HeadBufferIndex, ByteInt, 0, 4);
                }

                DataLength = BitConverter.ToInt32(ByteInt, 0);
                return Length >= DataLength;
            }
        }

        private void GetFreeBuffer()
        {
            lock (FreeBufferPool)
            {
                if (FreeBufferPool.Count == 0)
                {
                    for (int i = 0; i != 20; i++)
                        FreeBufferPool.Push(new byte[BufferElementSize]);
                }
                BufferFilled.Enqueue(FreeBufferPool.Pop());
                TailBufferIndex = 0;
            }
        }

        private void RevertBuffer()
        {
            lock (FreeBufferPool)
            {
                byte[] buff = null;

                if (BufferFilled.Count == 1)
                {
                    buff = BufferFilled.First();
                    TailBufferIndex = 0;
                }
                else
                {
                    buff = BufferFilled.Dequeue();
                    FreeBufferPool.Push(buff);
                }
                Array.Clear(buff, 0, BufferElementSize);

                HeadBufferIndex = 0;

                if (FreeBufferPool.Count > 30)
                {
                    for (int i = FreeBufferPool.Count - 10; i != 0; i++)
                        FreeBufferPool.Pop();
                }
            }
        }
    }

    static class GlobalObjectPool
    {
        public static void InitializePool()
        {
            ManualResetEventPool = new ObjectPool<ManualResetEvent>(
                20,
                20,
                () => { return new ManualResetEvent(false); },
                (e) => { e.Reset(); },
                (e) => { e.Dispose(); }
            );
        }

        public static void DestroyPool()
        {
            ManualResetEventPool.DestroyPool();
        }

        public static ObjectPool<ManualResetEvent> ManualResetEventPool { get; private set; }
    }

    class ObjectPool<T>
        where T : class
    {
        private int m_MaxFreeSize;

        private Func<T> m_Construct;

        private Action<T> m_Destruct;

        private Action<T> m_ResetState;

        private Stack<T> m_Pool;

        private object Lock = new object();

        public ObjectPool(int InitSize, int MaxFreeSize, Func<T> Construct, Action<T> ResetState, Action<T> Destruct)
        {
            if (InitSize < 0 || MaxFreeSize < 0 || Construct == null)
                throw new ArgumentException();

            m_Pool = new Stack<T>();
            m_MaxFreeSize = MaxFreeSize;
            m_Construct = Construct;
            m_Destruct = Destruct;
            m_ResetState = ResetState;

            for (int i = 0; i != InitSize; i++)
            {
                T t = m_Construct();
                m_ResetState?.Invoke(t);
                m_Pool.Push(t);
            }
        }

        public int Count
        {
            get
            {
                lock (Lock)
                    return m_Pool.Count;
            }
        }

        public void Push(T t)
        {
            if (t == null)
                throw new ArgumentNullException();

            lock (Lock)
            {
                if (m_Pool.Count == m_MaxFreeSize)
                    m_Destruct?.Invoke(t);
                else
                    m_Pool.Push(t);
            }
        }

        public T Pop()
        {
            lock (Lock)
            {
                if (m_Pool.Count == 0)
                    return m_Construct();
                else
                    return m_Pool.Pop();
            }
        }

        public void DestroyPool()
        {
            lock (Lock)
            {
                while (m_Pool.Count != 0)
                    m_Destruct?.Invoke(m_Pool.Pop());

                m_Pool = null;
            }
        }
    }
}
