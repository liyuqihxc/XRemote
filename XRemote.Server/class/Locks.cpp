#include "stdafx.h"
#include "hxc.h"

namespace hxc
{
	//////SRWLock
#pragma region SRWLock
	SRWLock::SRWLock(int SpinDelay)
	{
		
	}

	SRWLock::~SRWLock(void)
	{
		
	}

	void SRWLock::AcquireExclusive(int SpinCount/*=-1*/)
	{
		_ASSERT(SpinCount >= -1);
		BOOL Condition = SpinCount == -1 ? TRUE : SpinCount;
		while (Condition)
		{
			//检查是否有写操作占用锁
			
			{
				if (SpinCount > -1)
					Condition--;
				continue;
			}
		}
	}

	void SRWLock::AcquireShared(int SpinCount/*=-1*/)
	{
		
	}

	void SRWLock::ReleaseExclusive()
	{
		
	}

	void SRWLock::ReleaseShared()
	{
		
	}
#pragma endregion SRWLock

};	//namespace hxc