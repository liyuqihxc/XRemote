#include "stdafx.h"
#include "hxc.h"

namespace hxc
{
    QueueBuffer::QueueBuffer() : CurrentBufferIndex(0)
    {
        InitializeCriticalSection(&m_Lock);
    }

    QueueBuffer::~QueueBuffer()
    {
        Lock();
        if (CurrentBuffer != nullptr)
            _DataPool::BufferPool().Push(CurrentBuffer);
        while (!Buffers_Filled.empty())
        {
            _DataPool::BufferPool().Push(Buffers_Filled.front());
            Buffers_Filled.pop_front();
        }
        DeleteCriticalSection(&m_Lock);
    }

    void QueueBuffer::Lock(void)
    {
        EnterCriticalSection(&m_Lock);
    }

    void QueueBuffer::Write(const LPBYTE pNewData, int nLength)
    {
        if (CurrentBuffer == nullptr)
            CurrentBuffer = _DataPool::BufferPool().Pop();

        int lenPartData = _DataPool::BufferSize - CurrentBufferIndex;
        
        if (lenPartData > nLength)
        {
            //当前缓冲区足够放下pNewData
            memcpy_s(&CurrentBuffer[CurrentBufferIndex], lenPartData, pNewData, nLength);
            CurrentBufferIndex += nLength;
        }
        else if (lenPartData < nLength)
        {
            memcpy_s(&CurrentBuffer[CurrentBufferIndex], lenPartData, pNewData, lenPartData);
            Buffers_Filled.push_back(CurrentBuffer);
            CurrentBuffer = nullptr;
            CurrentBufferIndex = 0;
            Write(&pNewData[lenPartData], nLength - lenPartData);
        }
        else
        {
            memcpy_s(&CurrentBuffer[CurrentBufferIndex], lenPartData, pNewData, nLength);
            Buffers_Filled.push_back(CurrentBuffer);
            CurrentBuffer = nullptr;
            CurrentBufferIndex = 0;
        }
    }

    void QueueBuffer::SynchronizedWrite(const LPBYTE pNewData, int Length)
    {
        Lock();
        Write(pNewData, Length);
        Release();
    }

    int QueueBuffer::Read(LPBYTE pBuff, int Length)
    {
        if (Buffers_Filled.empty() && (CurrentBuffer == nullptr || CurrentBufferIndex == HeadBufferIndex))
            return 0;

        int nCount = 0;
        BYTE* ptr = pBuff;
        int length = Length;

        while (length != 0)
        {
            int headLength = Buffers_Filled.size() == 0 ? // 以缓冲区最长为 10 |- - - - 0 0 0 0 0 0|
                CurrentBufferIndex :
                _DataPool::BufferSize - HeadBufferIndex;// 计算Buffer头所在缓冲区的数据长度
            int n = length <= headLength ? length : headLength;// 确认请求的长度是否大于当前Buffer头所在缓冲区的数据长度
            BYTE* front = Buffers_Filled.size() != 0 ? Buffers_Filled.front() : CurrentBuffer;
            if (front == nullptr) break;
            memcpy_s(ptr, length, &front[HeadBufferIndex], n);
            HeadBufferIndex += n;
            ptr = &ptr[n];
            length -= n;
            nCount += n;
            if (Buffers_Filled.size() == 0 && HeadBufferIndex == CurrentBufferIndex)
            {
                break;
            }
            else if (Buffers_Filled.size() != 0 && HeadBufferIndex == _DataPool::BufferSize)
            {
                _DataPool::BufferPool().Push(Buffers_Filled.front());
                Buffers_Filled.pop_front();
                HeadBufferIndex = 0;
            }
        }
        return nCount;
    }

    int QueueBuffer::SynchronizedRead(LPBYTE pNewData, int Length)
    {
        Lock();
        int ret = Read(pNewData, Length);
        Release();
        return ret;
    }

    void QueueBuffer::Release(void)
    {
        LeaveCriticalSection(&m_Lock);
    }

    /*int QueueBuffer::Peek(BYTE * pBuff, int Length)
    {
        if (Buffers_Filled.empty() && CurrentBuffer == nullptr)
            return 0;

        int nCount = 0;
        BYTE* ptr = pBuff;
        int length = Length;
        int headIndex = HeadIndex;

        auto iterator = Buffers_Filled.begin();
        while (length != 0)
        {
            int headLength = headIndex == 0 ? // 以缓冲区最长为 10 |- - - - 0 0 0 0 0 0|
                CurrentBufferIndex + 1 :
                _DataPool::BufferSize - headIndex - 1;// 计算Buffer头所在缓冲区的数据长度
            int n = length <= headLength ? length : headLength;// 确认请求的长度是否大于当前Buffer头所在缓冲区的数据长度
            BYTE* front = iterator != Buffers_Filled.end() ? *iterator : CurrentBuffer;
            if (front == nullptr) break;
            memcpy_s(ptr, length, &front[headIndex], n);
            headIndex += n;
            length -= n;
            nCount += n;
            if (headIndex >= _DataPool::BufferSize)
            {
                iterator++;
                headIndex = 0;
            }
        }
        return nCount;
    }*/

    unsigned long QueueBuffer::get__Length(void) const
    {
        unsigned long len = 0;
        if (Buffers_Filled.size() == 0)
        {
            len = CurrentBufferIndex - HeadBufferIndex;
        }
        else
        {
            len = Buffers_Filled.size() * _DataPool::BufferSize - (HeadBufferIndex + 1);
            len += CurrentBufferIndex + 1;
        }
        
        return len;
    }
}
