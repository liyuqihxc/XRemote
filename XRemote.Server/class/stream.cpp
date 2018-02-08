#include "stdafx.h"
#include "hxc.h"

namespace hxc
{
namespace io
{
    ZeroCopyBuffer::ZeroCopyBuffer(std::shared_ptr<BYTE>& buffer, int offset, int size) :
        _buffer_size(size), _buffer(buffer), _buffer_start_index(offset)//, _data_size(0)
    {
        
    }

    ZeroCopyBuffer::ZeroCopyBuffer() :
        _buffer_size(0), _buffer(nullptr), _buffer_start_index(0)//, _data_size(0)
    {

    }

    ZeroCopyBuffer::ZeroCopyBuffer(const ZeroCopyBuffer & o)
    {
        _buffer = o._buffer;
        _buffer_size = o._buffer_size;
        //_data_size = o._data_size;
    }

    ZeroCopyBuffer & ZeroCopyBuffer::operator=(const ZeroCopyBuffer & o)
    {
        if (this != &o)
        {
            _buffer = o._buffer;
            _buffer_size = o._buffer_size;
            //_data_size = o._data_size;
        }
        return *this;
    }

    ZeroCopyStream::ZeroCopyStream() :
        _eback(nullptr), _gptr(nullptr), _egptr(nullptr),
        _pbase(nullptr), _pptr(nullptr), _epptr(nullptr)
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
        return buffer.get__BufferSize();
    }

    int ZeroCopyStream::DoWrite(ZeroCopyBuffer& buffer)
    {
        Task t = DoWriteAsync(buffer);
        t.Wait();
        return t.get__Result();
    }

    Task ZeroCopyStream::Flush(void)
    {
        return Task::get__CompletedTask();
    }
}//namespace io
}//namespace hxc