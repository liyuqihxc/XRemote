#include "stdafx.h"
#include "hxc.h"
#include <Ws2tcpip.h>
#include "Res.h"
#include <algorithm>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace hxc
{
namespace rpc
{
    HRESULT _TypeInfoHolder::GetTI(LCID lcid, ITypeInfo ** ppInfo)
    {
        _ASSERT(ppInfo != NULL);
        if (ppInfo == NULL)
            return E_POINTER;

        HRESULT hr = S_OK;
        if (m_pInfo == NULL)
            hr = GetTI(lcid);
        *ppInfo = m_pInfo;
        if (m_pInfo != NULL)
        {
            m_pInfo->AddRef();
            hr = S_OK;
        }
        return hr;
    }

    HRESULT _TypeInfoHolder::GetTI(LCID lcid)
    {
        //If this assert occurs then most likely didn't initialize properly
        _ASSERT(m_pguid != NULL);

        if (m_pInfo != NULL && m_pMap != NULL)
            return S_OK;

        HRESULT hRes = E_FAIL;
        if (m_pInfo == NULL)
        {
            ITypeLib* pTypeLib = NULL;
            {
                TCHAR szFilePath[MAX_PATH];
                DWORD dwFLen = ::GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), szFilePath, MAX_PATH);
                if (dwFLen != 0 && dwFLen != MAX_PATH)
                {
                    LPOLESTR pszFile = szFilePath;
                    hRes = LoadTypeLib(pszFile, &pTypeLib);
                }
            }
            if (SUCCEEDED(hRes))
            {
                ITypeInfo *pTypeInfo;
                hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &pTypeInfo);
                if (SUCCEEDED(hRes))
                {
                    ITypeInfo2 *pTypeInfo2;
                    if (SUCCEEDED(pTypeInfo->QueryInterface(IID_PPV_ARGS(&pTypeInfo2))))
                    {
                        m_pInfo = pTypeInfo2;
                        pTypeInfo->Release();
                    }
                    else
                    {
                        m_pInfo = pTypeInfo;
                    }
                }
                pTypeLib->Release();
            }
        }
        else
        {
            hRes = S_OK;
        }

        if (m_pInfo != NULL && m_pMap == NULL)
        {
            hRes = LoadNameCache(m_pInfo);
        }

        return hRes;
    }

    HRESULT _TypeInfoHolder::EnsureTI(LCID lcid)
    {
        HRESULT hr = S_OK;
        if (m_pInfo == NULL || m_pMap == NULL)
            hr = GetTI(lcid);
        return hr;
    }

    void _TypeInfoHolder::Cleanup()
    {
        if (m_pInfo != NULL)
            m_pInfo->Release();
        m_pInfo = NULL;
        delete[] m_pMap;
        m_pMap = NULL;
    }

    HRESULT _TypeInfoHolder::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
    {
        if (itinfo != 0)
        {
            return DISP_E_BADINDEX;
        }
        return GetTI(lcid, pptinfo);
    }

    HRESULT _TypeInfoHolder::GetIDsOfNames(REFIID, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
    {
        HRESULT hRes = EnsureTI(lcid);
        _Analysis_assume_(m_pInfo != NULL || FAILED(hRes));
        if (m_pInfo != NULL)
        {
            hRes = E_FAIL;
            // Look in cache if
            //	cache is populated
            //	parameter names are not requested
            if (m_pMap != NULL && cNames == 1)
            {
                int n = int(wcslen(rgszNames[0]));
                for (int j = m_nCount - 1; j >= 0; j--)
                {
                    if ((n == m_pMap[j].nLen) &&
                        (memcmp(m_pMap[j].bstr, rgszNames[0], m_pMap[j].nLen * sizeof(OLECHAR)) == 0))
                    {
                        rgdispid[0] = m_pMap[j].id;
                        hRes = S_OK;
                        break;
                    }
                }
            }
            // if cache is empty or name not in cache or parameter names are requested,
            // delegate to ITypeInfo::GetIDsOfNames
            if (FAILED(hRes))
            {
                hRes = m_pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
            }
        }
        return hRes;
    }

    HRESULT _TypeInfoHolder::Invoke(IDispatch * p, DISPID dispidMember, REFIID, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
    {
        HRESULT hRes = EnsureTI(lcid);
        _Analysis_assume_(m_pInfo != NULL || FAILED(hRes));
        if (m_pInfo != NULL)
            hRes = m_pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
        return hRes;
    }

    HRESULT _TypeInfoHolder::LoadNameCache(ITypeInfo * pTypeInfo)
    {
        TYPEATTR* pta;
        HRESULT hr = pTypeInfo->GetTypeAttr(&pta);
        if (SUCCEEDED(hr))
        {
            stringdispid* pMap = NULL;
            m_nCount = pta->cFuncs;
            m_pMap = NULL;
            if (m_nCount != 0)
            {
                pMap = new stringdispid[m_nCount];
                if (pMap == NULL)
                {
                    pTypeInfo->ReleaseTypeAttr(pta);
                    return E_OUTOFMEMORY;
                }
            }
            for (int i = 0; i < m_nCount; i++)
            {
                FUNCDESC* pfd;
                if (SUCCEEDED(pTypeInfo->GetFuncDesc(i, &pfd)))
                {
                    BSTR bstrName;
                    if (SUCCEEDED(pTypeInfo->GetDocumentation(pfd->memid, &bstrName, NULL, NULL, NULL)))
                    {
                        pMap[i].bstr = bstrName;
                        pMap[i].nLen = SysStringLen(pMap[i].bstr);
                        pMap[i].id = pfd->memid;
                    }
                    pTypeInfo->ReleaseFuncDesc(pfd);
                }
            }
            m_pMap = pMap;
            pTypeInfo->ReleaseTypeAttr(pta);
        }
        return S_OK;
    }

    ZeroCopyNetworkInputStream::ZeroCopyNetworkInputStream(tcp_stream & tcp_stream, int capacity) :
        _buffer((uint8_t*)malloc(capacity)), _capacity(capacity), _tcp_stream(tcp_stream), _signal(CreateEvent(nullptr, true, false, nullptr)),
        _head(0), _ptr(0), _recv_task(Task::BindFunction(&ZeroCopyNetworkInputStream::ReceiveProc, this), NULL)
    {
        _recv_task.Start();
    }

    ZeroCopyNetworkInputStream::~ZeroCopyNetworkInputStream()
    {
        _recv_task.Cancel();

        if (_capacity)
            free(_buffer);
    }

    bool ZeroCopyNetworkInputStream::Next(const void ** data, int * size)
    {
        _head = -1;
        ::ResetEvent(_signal);

        if (::WaitForSingleObject(_signal, INFINITE) == WAIT_FAILED)
            return false;

        *data = &_buffer[_head];
        *size = _ptr - _head;
        return true;
    }

    void ZeroCopyNetworkInputStream::BackUp(int count)
    {
        // NetworkStream不支持BackUp方法
        NotSupportedException e;
        SET_EXCEPTION(e);
        throw e;
    }

    bool ZeroCopyNetworkInputStream::Skip(int count)
    {
        // NetworkStream不支持Skip方法
        NotSupportedException e;
        SET_EXCEPTION(e);
        throw e;
    }

    long long ZeroCopyNetworkInputStream::ByteCount() const
    {
        return 0;
    }

    DWORD_PTR ZeroCopyNetworkInputStream::ReceiveProc(DWORD_PTR Param, HANDLE hCancel)
    {
        HANDLE signals[] = { hCancel, _signal };
        DWORD wait_result = 0;
        while (wait_result = ::WaitForMultipleObjects(2, signals, false, 0))
        {
            if (wait_result == WAIT_OBJECT_0) // Task 取消
                break;
            else if (wait_result == WAIT_FAILED)
                return -1;
            else if (wait_result == WAIT_OBJECT_0 + 1) // ZeroCopyNetworkInputStream::_signal无信号的状态时才可以接收数据
                continue;

            if (_ptr == _capacity)
                _ptr = _head = 0;

            _head = _ptr;
            _ptr += _tcp_stream.Read(_buffer, _ptr, _capacity - _ptr);
            ::SetEvent(_signal);
        }
        return 0;
    }

    LengthDelimitedNetworkInputStream::LengthDelimitedNetworkInputStream(tcp_stream & netstream, LPBYTE pBuffer, int size) :
        _byte_count(0), _stream(netstream), _direct_buffer(pBuffer), _buffer_size(size), _remain_size(0),
        _recv_task(Task::BindFunction(&LengthDelimitedNetworkInputStream::ReceiveProc, this), NULL)
    {
        _signal = ::CreateEvent(nullptr, true, false, nullptr);
        _recv_task.Start();
    }

    LengthDelimitedNetworkInputStream::~LengthDelimitedNetworkInputStream()
    {
        _recv_task.Cancel();
        _DataPool::BufferPool().Push(_direct_buffer);
        ::CloseHandle(_signal);
    }

    bool LengthDelimitedNetworkInputStream::Next(const void ** data, int * size)
    {
        ::ResetEvent(_signal);
        if (::WaitForSingleObject(_signal, INFINITE) == WAIT_OBJECT_0)
        {
            *data = _direct_buffer;
            *size = static_cast<int>(_current_size);
            return true;
        }
        return false;
    }

    void LengthDelimitedNetworkInputStream::BackUp(int count)
    {
        // LengthDelimited的方式不需要用到BackUp操作
    }

    bool LengthDelimitedNetworkInputStream::Skip(int count)
    {
        ::ResetEvent(_signal);
        return false;
    }

    long long LengthDelimitedNetworkInputStream::ByteCount() const
    {
        ::ResetEvent(_signal);
        return 0;
    }

    bool LengthDelimitedNetworkInputStream::ReadDelimitedFrom(google::protobuf::io::ZeroCopyInputStream * rawInput, google::protobuf::MessageLite & message)
    {
        // We create a new coded stream for each message.  Don't worry, this is fast,
        // and it makes sure the 64MB total size limit is imposed per-message rather
        // than on the whole stream.  (See the CodedInputStream interface for more
        // info on this limit.)
        google::protobuf::io::CodedInputStream input(rawInput);

        // Read the size.
        uint32_t size;
        if (!input.ReadVarint32(&size))
            return false;

        // Tell the stream not to read beyond that size.
        google::protobuf::io::CodedInputStream::Limit limit =
            input.PushLimit(size);

        // Parse the message.
        if (!message.MergeFromCodedStream(&input))
            return false;
        if (!input.ConsumedEntireMessage())
            return false;

        // Release the limit.
        input.PopLimit(limit);

        return true;
    }

    DWORD_PTR LengthDelimitedNetworkInputStream::ReceiveProc(DWORD_PTR Param, HANDLE hCancel)
    {
        using namespace google::protobuf::io;

        HANDLE signals[] = { hCancel, _signal };
        DWORD wait_result = 0;
        while (wait_result = ::WaitForMultipleObjects(2, signals, false, 0))
        {
            if (wait_result == WAIT_OBJECT_0) // Task 取消
                break;
            else if (wait_result == WAIT_FAILED)
                return -1;
            else if (wait_result == WAIT_OBJECT_0 + 1) // LengthDelimitedNetworkInputStream::_signal无信号的状态时才可以接收数据
                continue;

            _current_size = 0;
            uint32_t data_size = 0;

            if (_remain_size != 0)
            {
                while (::WaitForSingleObject(hCancel, 0) == WAIT_TIMEOUT && data_size != (std::min)(_buffer_size, _remain_size))
                    data_size += _stream.Read(_direct_buffer, data_size, (std::min)(_buffer_size, _remain_size) - data_size);

                _current_size = data_size;
                _remain_size -= data_size;
            }
            else
            {
                while (::WaitForSingleObject(hCancel, 0) == WAIT_TIMEOUT && data_size != 4)
                    data_size += _stream.Read(_direct_buffer, data_size, 1);

                ArrayInputStream ais(_direct_buffer, _buffer_size);
                CodedInputStream cis(&ais);
                if (!cis.ReadVarint32(&_remain_size))
                    continue;
                
                uint32_t length_size = CodedOutputStream::VarintSize32(_remain_size);
                _remain_size += length_size;
                while (::WaitForSingleObject(hCancel, 0) == WAIT_TIMEOUT && data_size != (std::min)(_buffer_size - data_size, _remain_size))
                    data_size += _stream.Read(_direct_buffer, data_size, (std::min)(_buffer_size - data_size, _remain_size) - data_size);

                _current_size = data_size;
                if (_remain_size > _buffer_size)
                    _remain_size -= data_size;
                else
                    _remain_size = 0;
            }

            ::SetEvent(_signal);
        }
        return 0;
    }

    bool LengthDelimitedNetworkOutputStream::WriteDelimitedTo(const google::protobuf::MessageLite & message, google::protobuf::io::ZeroCopyOutputStream * rawOutput)
    {
        // We create a new coded stream for each message.  Don't worry, this is fast.
        google::protobuf::io::CodedOutputStream output(rawOutput);

        // Write the size.
        const int size = message.ByteSize();
        output.WriteVarint32(size);

        uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
        if (buffer != NULL) {
            // Optimization:  The message fits in one buffer, so use the faster
            // direct-to-array serialization path.
            message.SerializeWithCachedSizesToArray(buffer);
        }
        else {
            // Slightly-slower path when the message is multiple buffers.
            message.SerializeWithCachedSizes(&output);
            if (output.HadError()) return false;
        }

        return true;
    }

};//namespace rpc
};//namespace hxc
