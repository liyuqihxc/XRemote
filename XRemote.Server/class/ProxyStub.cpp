#include "stdafx.h"
#include "hxc.h"
#include <Ws2tcpip.h>
#include "Res.h"
#include <algorithm>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace hxc
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

};//namespace hxc
