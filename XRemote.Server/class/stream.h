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
        ZeroCopyBuffer(std::shared_ptr<BYTE>& buffer, int offset, int size);
    public:
        ZeroCopyBuffer();
        ZeroCopyBuffer(const ZeroCopyBuffer& o);
        ZeroCopyBuffer& operator=(const ZeroCopyBuffer& o);
        LPBYTE get__BufferPointer() { return &_buffer.get()[_buffer_start_index]; }
        int get__BufferSize() const { return _buffer_size; }
        //void set__DataSize(int DataSize) { _data_size = DataSize; }
        //int get__DataSize() const { return _data_size; }
    private:
        std::shared_ptr<BYTE> _buffer;
        int _buffer_start_index;
        int _buffer_size;
        //int _data_size;
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

        virtual ZeroCopyBuffer StartWrite() = 0;
        int DoWrite(ZeroCopyBuffer& buffer);
        virtual Task DoWriteAsync(ZeroCopyBuffer& buffer) = 0;

        virtual Task Flush(void);
    protected:
        CRITICAL_SECTION _SyncRoot;

        LPBYTE _eback;
        LPBYTE _gptr;
        LPBYTE _egptr;

        LPBYTE _pbase;
        LPBYTE _pptr;
        LPBYTE _epptr;
    };
}
}

#endif // !_STREAM_H_

