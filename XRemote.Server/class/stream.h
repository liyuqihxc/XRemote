#pragma once

#ifndef _STREAM_H_
#define _STREAM_H_

namespace hxc
{
namespace io
{
    class ZeroCopyBuffer
    {
    private:
        friend class ZeroCopyStream;
        ZeroCopyBuffer(std::shared_ptr<BYTE>& Buffer, int BufferSize);
    public:
        ZeroCopyBuffer();
        ZeroCopyBuffer(const ZeroCopyBuffer& o);
        ZeroCopyBuffer& operator=(const ZeroCopyBuffer& o);
        LPBYTE get__BufferPointer() { return _buffer.get(); }
        int get__BufferSize() const { return _buffer_size; }
        void set__DataSize(int DataSize) { _data_size = DataSize; }
        int get__DataSize() const { return _data_size; }
    private:
        std::shared_ptr<BYTE> _buffer;
        int _buffer_size;
        int _data_size;
    };

    class ZeroCopyStream
    {
    private:
        ZeroCopyStream(const ZeroCopyStream&) = delete;
        ZeroCopyStream& operator=(const ZeroCopyStream&) = delete;
    public:
        ZeroCopyStream();
        virtual ~ZeroCopyStream();

        void AcquireLock(void) { ::EnterCriticalSection(&_SyncRoot); }
        void ReleaseLock(void) { ::LeaveCriticalSection(&_SyncRoot); }

        int Read(int count, ZeroCopyBuffer& buffer);
        virtual Task ReadAsync(int count, ZeroCopyBuffer& buffer) = 0;

        ZeroCopyBuffer StartWrite();
        int DoWrite(ZeroCopyBuffer& buffer);
        virtual Task DoWriteAsync(ZeroCopyBuffer& buffer) = 0;

        virtual void Flush(void);
    protected:
        CRITICAL_SECTION _SyncRoot;
        ObjectPool<LPBYTE> _FreeBuffers;
    };
}
}

#endif // !_STREAM_H_

