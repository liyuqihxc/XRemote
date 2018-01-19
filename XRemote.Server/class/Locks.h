#pragma once

#if !defined _HXC_LOCK_H_
#define _HXC_LOCK_H_

namespace hxc
{
	class SRWLock
	{
	public:
		explicit SRWLock(int SpinDelay = 0);
		~SRWLock(void);
		void AcquireExclusive(int SpinCount = -1);//旋转次数默认为-1，即一直重试直到获取锁。
		void AcquireShared(int SpinCount = -1);
		void ReleaseExclusive(void);
		void ReleaseShared(void);
	private:
		
	};


    template<class T>
    class _declspec(novtable) ThreadSafeObjectBase
    {
    public:
        ThreadSafeObjectBase() {}
        explicit ThreadSafeObjectBase(const T& t) : m_Object(t) {}
        virtual ~ThreadSafeObjectBase() {}
        T* operator->() { return &this->m_Object; }
        virtual void Lock() = 0;
        virtual void Release() = 0;
    protected:
        T m_Object;
    };

    template<class T>
    class Critical_Section : public ThreadSafeObjectBase<T>
    {
    public:
        Critical_Section(void) : m_Destructor(nullptr)
        {
            ::InitializeCriticalSection(&this->m_Lock);
        }

        Critical_Section(std::function<T(void)> Constructor, std::function<void(T&)> Destructor) :
            m_Destructor(Destructor), ThreadSafeObjectBase(Constructor())
        {
            
        }

        virtual ~Critical_Section(void)
        {
            if (m_Destructor)
                m_Destructor(m_Object);

            ::DeleteCriticalSection(&this->m_Lock);
        }
        virtual void Lock(void)
        {
            ::EnterCriticalSection(&this->m_Lock);
        }
        virtual void Release(void)
        {
            ::LeaveCriticalSection(&this->m_Lock);
        }
    protected:
        CRITICAL_SECTION m_Lock;
        std::function<void(T&)> m_Destructor;
    };

};	//namespace hxc

#endif		//if !defined _HXC_LOCK_H_