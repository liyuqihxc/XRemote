#pragma once

#if !defined _MEMORYSTREAM_H_
#define _MEMORYSTREAM_H_

namespace hxc
{
    enum SeekOrigin
    {
        Begin,
        Current,
        End
    };

    class MemoryStream
    {
    public:
        MemoryStream();
        MemoryStream(const MemoryStream& o);
        virtual ~MemoryStream();
    public:
        MemoryStream & operator=(const MemoryStream& o);
        int Read(LPBYTE buffer, int offset, int count);
        LARGE_INTEGER Seek(LARGE_INTEGER offset, SeekOrigin loc);
        int Write(const LPBYTE buffer, int offset, int count);
    protected:
    private:
    };

    class QueueBuffer
    {
    public:
        QueueBuffer();
        ~QueueBuffer();
        void Lock(void);
        void Write(const LPBYTE pNewData, int Length);
        void SynchronizedWrite(const LPBYTE pNewData, int Length);
        int Read(LPBYTE pBuff, int Length);
        int SynchronizedRead(LPBYTE pNewData, int Length);
        void Release(void);
        //int Peek(BYTE*pBuff, int Length);
        unsigned long get__Length(void) const;
    private:
        LPBYTE CurrentBuffer;
        long CurrentBufferIndex;

        long HeadBufferIndex;// Buffer头索引

        std::list<LPBYTE> Buffers_Filled;
        CRITICAL_SECTION m_Lock;
    };
}

#endif // !defined _MEMORYSTREAM_H_
