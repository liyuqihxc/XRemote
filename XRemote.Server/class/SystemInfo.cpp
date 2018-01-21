#include "stdafx.h"
#include "hxc.h"
#include <Winperf.h>

namespace hxc
{
    std::wstring SystemInfo::_OSName;
    const std::wstring & SystemInfo::get__OSName()
    {
        if (_OSName.empty())
        {
            HKEY hKey;
            LONG lRet;
            DWORD dwLength = 0;

            lRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);
            lRet = RegQueryValueExW(hKey, L"ProductName", nullptr, nullptr, nullptr, &dwLength);
            LPWSTR lpValue = new WCHAR[dwLength];
            lRet = RegQueryValueExW(hKey, L"ProductName", nullptr, nullptr, (LPBYTE)lpValue, &dwLength);
            std::wstring osName = lpValue;
            delete[] lpValue;
            dwLength = 0;

            lRet = RegQueryValueExW(hKey, L"CSDVersion", nullptr, nullptr, nullptr, &dwLength);
            if (ERROR_SUCCESS == lRet)
            {
                lpValue = new WCHAR[dwLength];
                lRet = RegQueryValueExW(hKey, L"CSDVersion", nullptr, nullptr, (LPBYTE)lpValue, &dwLength);
                osName += L" ";
                osName += lpValue;
                delete[] lpValue;
            }
            RegCloseKey(hKey);

            _OSName = osName;
        }
        return _OSName;
    }

    BOOL SystemInfo::get__IsWow64()
    {
        typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
        LPFN_ISWOW64PROCESS fnIsWow64Process;
        BOOL bIsWow64 = FALSE;
        fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
        if (NULL != fnIsWow64Process)
        {
            fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
        }
        return bIsWow64;
    }

    int SystemInfo::get__NetworkInterFaces()
    {
        try
        {
#define DEFAULT_BUFFER_SIZE 40960L  

            unsigned char *data = (unsigned char*)malloc(DEFAULT_BUFFER_SIZE);
            DWORD type;
            DWORD size = DEFAULT_BUFFER_SIZE;
            DWORD ret;

            wchar_t s_key[4096];
            swprintf_s(s_key, 4096, L"510");
            //RegQueryValueEx的固定调用格式          
            std::wstring str(s_key);

            //如果RegQueryValueEx函数执行失败则进入循环  
            while ((ret = RegQueryValueEx(HKEY_PERFORMANCE_DATA, str.c_str(), 0, &type, data, &size)) != ERROR_SUCCESS)
            {
                Sleep(10);
                //如果RegQueryValueEx的返回值为ERROR_MORE_DATA(申请的内存区data太小，不能容纳RegQueryValueEx返回的数据)  
                if (ret == ERROR_MORE_DATA)
                {
                    Sleep(10);
                    size += DEFAULT_BUFFER_SIZE;
                    data = (unsigned char*)realloc(data, size);//重新分配足够大的内存  

                    ret = RegQueryValueEx(HKEY_PERFORMANCE_DATA, str.c_str(), 0, &type, data, &size);//重新执行RegQueryValueEx函数  
                }
                //如果RegQueryValueEx返回值仍旧未成功则函数返回.....(注意内存泄露“free函数”~~~)。  
                //这个if保证了这个while只能进入一次~~~避免死循环  
                if (ret != ERROR_SUCCESS)
                {
                    if (NULL != data)
                    {
                        free(data);
                        data = NULL;
                    }
                    return 0;//0个接口  
                }
            }

            //函数执行成功之后就是对返回的data内存中数据的解析了，这个建议去查看MSDN有关RegQueryValueEx函数参数数据结构的说明  
            //得到数据块       
            PERF_DATA_BLOCK  *dataBlockPtr = (PERF_DATA_BLOCK *)data;
            //得到第一个对象  
            PERF_OBJECT_TYPE *objectPtr = (PERF_OBJECT_TYPE *)((BYTE *)dataBlockPtr + dataBlockPtr->HeaderLength);

            for (int a = 0; a < (int)dataBlockPtr->NumObjectTypes; a++)
            {
                char nameBuffer[255] = { 0 };
                if (objectPtr->ObjectNameTitleIndex == 510)
                {
                    DWORD processIdOffset = ULONG_MAX;
                    PERF_COUNTER_DEFINITION *counterPtr = (PERF_COUNTER_DEFINITION *)((BYTE *)objectPtr + objectPtr->HeaderLength);

                    for (int b = 0; b < (int)objectPtr->NumCounters; b++)
                    {
                        if (counterPtr->CounterNameTitleIndex == 520)
                            processIdOffset = counterPtr->CounterOffset;

                        counterPtr = (PERF_COUNTER_DEFINITION *)((BYTE *)counterPtr + counterPtr->ByteLength);
                    }

                    if (processIdOffset == ULONG_MAX) {
                        if (data != NULL)
                        {
                            free(data);
                            data = NULL;
                        }
                        return 0;
                    }

                    PERF_INSTANCE_DEFINITION *instancePtr = (PERF_INSTANCE_DEFINITION *)((BYTE *)objectPtr + objectPtr->DefinitionLength);

                    for (int b = 0; b < objectPtr->NumInstances; b++)
                    {
                        wchar_t *namePtr = (wchar_t *)((BYTE *)instancePtr + instancePtr->NameOffset);
                        PERF_COUNTER_BLOCK *counterBlockPtr = (PERF_COUNTER_BLOCK *)((BYTE *)instancePtr + instancePtr->ByteLength);

                        //char pName[256] = { 0 };
                        //WideCharToMultiByte(CP_ACP, 0, namePtr, -1, pName, sizeof(nameBuffer), 0, 0);

                        DWORD bandwith = *((DWORD *)((BYTE *)counterBlockPtr + processIdOffset));
                        DWORD tottraff = 0;

                        //Interfaces.AddTail(CString(pName)); //各网卡的名称  
                        //Bandwidths.AddTail(bandwith);       //带宽  
                        //TotalTraffics.AddTail(tottraff);    // 流量初始化为0  

                        PERF_COUNTER_BLOCK  *pCtrBlk = (PERF_COUNTER_BLOCK *)((BYTE *)instancePtr + instancePtr->ByteLength);


                        instancePtr = (PERF_INSTANCE_DEFINITION *)((BYTE *)instancePtr + instancePtr->ByteLength + pCtrBlk->ByteLength);
                    }
                }
                objectPtr = (PERF_OBJECT_TYPE *)((BYTE *)objectPtr + objectPtr->TotalByteLength);
            }
            if (data != NULL)
            {
                free(data);
                data = NULL;
            }
        }
        catch (...)
        {
            return 0;
        }
        return 0;
    }
}//namespace hxc
