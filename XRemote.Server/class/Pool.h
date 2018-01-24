#pragma once

#if !defined _HXC_POOL_H_
#define _HXC_POOL_H_

namespace hxc
{
    template<typename T>
    class ObjectPool
    {
    private:
        ObjectPool(void) {}
        ObjectPool(const ObjectPool&) {}
    public:
        typedef T Type;
        typedef std::function<Type(void)> Construct;
        typedef std::function<void(Type&)> ResetState;
        typedef std::function<void(Type&)> Destruct;

        ObjectPool(int InitNum, int MaxFreeNum, const Construct& construct, const ResetState& resetstate, const Destruct& destruct) :
            m_Construct(construct), m_ResetState(resetstate), m_Destruct(destruct), m_MaxFreeNum(MaxFreeNum)
        {
            if (InitNum < 0 && MaxFreeNum < 0)
                throw ArgumentException();

            for (int i = 0; i < InitNum; i++)
            {
                Type t = m_Construct();
                if (m_ResetState != nullptr)
                    m_ResetState(t);
                m_Pool->push(t);
            }
        }

        ~ObjectPool()
        {
            while (m_Pool->size() != 0)
            {
                m_Destruct(m_Pool->top());
                m_Pool->pop();
            }
        }

        void Push(Type t)
        {
            m_Pool.Lock();
            if ((int)m_Pool->size() >= m_MaxFreeNum)
            {
                m_Destruct(t);
            }
            else
            {
                if (m_ResetState != nullptr)
                    m_ResetState(t);
                m_Pool->push(t);
            }
            m_Pool.Release();
        }

        Type Pop(void)
        {
            Type t;

            m_Pool.Lock();
            if (m_Pool->size() == 0)
            {
                t = m_Construct();
            }
            else
            {
                t = m_Pool->top();
                m_Pool->pop();
            }
            m_Pool.Release();

            return t;
        }

    private:
        Construct m_Construct;
        ResetState m_ResetState;
        Destruct m_Destruct;
        int m_MaxFreeNum;
        Critical_Section<std::stack<Type>> m_Pool;
    };

    class _DataPool
    {
    public:
        static ObjectPool<HANDLE>& ManualResetEventPool();
        static ObjectPool<SOCKET>& TCPSocketPool();
        static ObjectPool<BYTE*>& BufferPool();

	    static void Initialize();
	    static void Free();

        static const int BufferSize = 100;
    private:
        static ObjectPool<HANDLE>* sm_pManualResetEventPool;
        static ObjectPool<SOCKET>* sm_pTCPSocketPool;
        static ObjectPool<BYTE*>* sm_pBufferPool;
    };
};

#endif // if !defined _HXC_POOL_H_
