#include "stdafx.h"
#include "hxc.h"

namespace hxc
{
namespace io
{
    ZeroCopyBuffer::ZeroCopyBuffer(std::shared_ptr<BYTE>& Buffer, int BufferSize) :
        _buffer_size(BufferSize), _buffer(Buffer), _data_size(0)
    {
        
    }

    ZeroCopyBuffer::ZeroCopyBuffer() :
        _buffer_size(0), _buffer(nullptr), _data_size(0)
    {

    }

    ZeroCopyBuffer::ZeroCopyBuffer(const ZeroCopyBuffer & o)
    {
        _buffer = o._buffer;
        _buffer_size = o._buffer_size;
        _data_size = o._data_size;
    }

    ZeroCopyBuffer & ZeroCopyBuffer::operator=(const ZeroCopyBuffer & o)
    {
        if (this != &o)
        {
            _buffer = o._buffer;
            _buffer_size = o._buffer_size;
            _data_size = o._data_size;
        }
        return *this;
    }

    ZeroCopyStream::ZeroCopyStream() :
        _FreeBuffers(5, 5, []() { return _DataPool::BufferPool().Pop(); }, nullptr, [](BYTE*& p) { _DataPool::BufferPool().Push(p); })
    {
        ::InitializeCriticalSection(&_SyncRoot);
    }

    ZeroCopyStream::~ZeroCopyStream()
    {
        ::DeleteCriticalSection(&_SyncRoot);
    }

    int ZeroCopyStream::Read(int count, ZeroCopyBuffer & buffer)
    {
        Task t = ReadAsync(count, buffer);
        t.Wait();
        return buffer.get__DataSize();
    }

    ZeroCopyBuffer ZeroCopyStream::StartWrite()
    {
        return ZeroCopyBuffer(std::shared_ptr<BYTE>(_FreeBuffers.Pop(), [this](LPBYTE p) { _FreeBuffers.Push(p); }), _DataPool::BufferSize);
    }

    int ZeroCopyStream::DoWrite(ZeroCopyBuffer & buffer)
    {
        Task t = DoWriteAsync(buffer);
        t.Wait();
        return t.get__Result();
    }

    void ZeroCopyStream::Flush(void)
    {
    }
}//namespace io
}//namespace hxc