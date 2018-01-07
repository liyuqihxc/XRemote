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

            ZeroMemory(&m_Lock, sizeof(CRITICAL_SECTION));
            InitializeCriticalSection(&m_Lock);

            for (int i = 0; i < InitNum; i++)
            {
                Type t = m_Construct();
                if (m_ResetState != nullptr)
                    m_ResetState(t);
                m_Pool.push(t);
            }
        }

        ~ObjectPool()
        {
            DeleteCriticalSection(&m_Lock);

            while (m_Pool.size() != 0)
            {
                m_Destruct(m_Pool.top());
                m_Pool.pop();
            }
        }

        void Push(Type t)
        {
            if (m_ResetState != nullptr)
                m_ResetState(t);
            m_Pool.push(t);
            
            int free = m_Pool.size() - m_MaxFreeNum;
            for (int i = 0; i < free; i++)
            {
                m_Destruct(m_Pool.top());
                m_Pool.pop();
            }
        }

        Type Pop(void)
        {
            Type t;

            if (m_Pool.size() == 0)
            {
                t = m_Construct();
                if (m_ResetState != nullptr)
                    m_ResetState(t);
            }
            else
            {
                t = m_Pool.top();
                m_Pool.pop();
            }

            return t;
        }

    private:
        Construct m_Construct;
        ResetState m_ResetState;
        Destruct m_Destruct;
        int m_MaxFreeNum;
        std::stack<Type> m_Pool;
        CRITICAL_SECTION m_Lock;
    };

    class _DataPool
    {
    public:
        static inline ObjectPool<HANDLE>& ManualResetEventPool()
        {
            if (!sm_pManualResetEventPool)
                Initialize();
            return *sm_pManualResetEventPool;
        }
        static inline ObjectPool<SOCKET>& TCPSocketPool()
        {
            if (!sm_pTCPSocketPool)
                Initialize();
            return *sm_pTCPSocketPool;
        }
        static inline ObjectPool<BYTE*>& BufferPool()
        {
            if (!sm_pBufferPool)
                Initialize();
            return *sm_pBufferPool;
        }

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
