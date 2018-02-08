#include "stdafx.h"
#include "hxc.h"
#include "ProxyStub_imp.h"
#include <Ws2tcpip.h>
#include "Res.h"

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

    NetworkInputStream::NetworkInputStream(tcp_stream & netstream) :
        _stream(netstream), _direct_buffer(_DataPool::BufferPool().Pop()),
        _head(0), _ptr(0), _end(0),
        _backup_direct_buffer(_DataPool::BufferPool().Pop()),
        _backup_head(0), _backup_ptr(0), _backup_end(0),
        _recv_task(Task::BindFunction(&NetworkInputStream::ReceiveProc, this), NULL)
    {

    }

    NetworkInputStream::~NetworkInputStream()
    {
        _DataPool::BufferPool().Push(_direct_buffer);
        _DataPool::BufferPool().Push(_backup_direct_buffer);
    }

    bool NetworkInputStream::Next(const void ** data, int * size)
    {

        return true;
    }

    void NetworkInputStream::BackUp(int count)
    {
        
    }

    bool NetworkInputStream::ReadDelimitedFrom(google::protobuf::io::ZeroCopyInputStream * rawInput, google::protobuf::MessageLite & message)
    {
        // We create a new coded stream for each message.  Don't worry, this is fast,
        // and it makes sure the 64MB total size limit is imposed per-message rather
        // than on the whole stream.  (See the CodedInputStream interface for more
        // info on this limit.)
        google::protobuf::io::CodedInputStream input(rawInput);

        // Read the size.
        uint32_t size;
        if (!input.ReadVarint32(&size)) return false;

        // Tell the stream not to read beyond that size.
        google::protobuf::io::CodedInputStream::Limit limit =
            input.PushLimit(size);

        // Parse the message.
        if (!message.MergeFromCodedStream(&input)) return false;
        if (!input.ConsumedEntireMessage()) return false;

        // Release the limit.
        input.PopLimit(limit);

        return true;
    }

    DWORD_PTR NetworkInputStream::ReceiveProc(DWORD_PTR Param, HANDLE hCancel)
    {
        //_stream.ReadAsync()
    }

    bool NetworkOutputStream::WriteDelimitedTo(const google::protobuf::MessageLite & message, google::protobuf::io::ZeroCopyOutputStream * rawOutput)
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
