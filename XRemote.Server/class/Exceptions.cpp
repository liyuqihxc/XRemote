#include "stdafx.h"
#include "hxc.h"

namespace hxc
{
    Exception::Exception() : _HResult(S_OK), _LineNumber(-1)
    {

    }

    Exception::Exception(const Exception & e)
    {
        _HResult = e._HResult;
        _Message = e._Message;
        _SourceFileName = e._SourceFileName;
        _Function = e._Function;
        _LineNumber = e._LineNumber;
    }

    Exception & Exception::operator=(const Exception & e)
    {
        if (this != &e)
        {
            _HResult = e._HResult;
            _Message = e._Message;
            _SourceFileName = e._SourceFileName;
            _Function = e._Function;
            _LineNumber = e._LineNumber;
        }
        return *this;
    }

    Exception::Exception(HRESULT hresult) : _HResult(hresult),
        _LineNumber(-1)
    {
        
    }

    Exception::Exception(const std::wstring & Message) :
        _Message(Message), _HResult(E_FAIL), _LineNumber(-1)
    {

    }

    HRESULT Exception::get__HResult() const { return _HResult; }

    void Exception::set__HResult(HRESULT value) { _HResult = value; }

    const std::wstring & Exception::get__Message() const { return _Message; }

    void Exception::set__Message(const std::wstring & value) { _Message = value; }

    const std::wstring & Exception::get__SourceFileName() const { return _SourceFileName; }

    void Exception::set__SourceFileName(const std::wstring & value) { _SourceFileName = value; }

    void Exception::set__SourceFileName(const std::string & value)
    {
        int num = MultiByteToWideChar(CP_ACP, 0, value.c_str(), value.size(), nullptr, 0);
        LPWSTR str = new WCHAR[num];
        MultiByteToWideChar(CP_ACP, 0, value.c_str(), value.size(), str, num);
        _SourceFileName = str;
        delete[] str;
    }

    const std::wstring & Exception::get__Function() const { return _Function; }

    void Exception::set__Function(const std::wstring & value) { _Function = value; }

    void Exception::set__Function(const std::string & value)
    {
        int num = MultiByteToWideChar(CP_ACP, 0, value.c_str(), value.size(), nullptr, 0);
        LPWSTR str = new WCHAR[num];
        MultiByteToWideChar(CP_ACP, 0, value.c_str(), value.size(), str, num);
        _Function = str;
        delete[] str;
    }

    int Exception::get__LineNumber() const { return _LineNumber; }

    void Exception::set__LineNumber(int value) { _LineNumber = value; }

    DWORD Exception::NTSTATUS_To_Win32Err(LONG code)
    {
        if (hNtdll == NULL)
        {
            hNtdll = LoadLibraryW(L"ntdll.dll");
            _NTSTATUS_To_Win32Err = (ULONG(WINAPI*)(LONG))GetProcAddress(hNtdll, "RtlNtStatusToDosError");
        }
        return _NTSTATUS_To_Win32Err(code);
    }

    HMODULE Exception::hNtdll;
    std::function<ULONG _stdcall(LONG)> Exception::_NTSTATUS_To_Win32Err;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    MemoryException::MemoryException(const std::wstring& Message) :
        Exception(Message)
    {

    }

    MemoryException::MemoryException(const std::bad_alloc& innerException) :
        Exception()
    {

    }

    NotImplementedException::NotImplementedException() : Exception()
    {
    }

    ArgumentException::ArgumentException() : Exception()
    {
    }

    FormatException::FormatException() : Exception()
    {
    }

    FormatException::FormatException(const std::wstring & Message) : Exception(Message)
    {
    }

    Win32Exception::Win32Exception(DWORD dwWin32Error) : Exception(HRESULT_FROM_WIN32(dwWin32Error))
    {
        _Message = FormatErrorMessage(dwWin32Error);
    }

    std::wstring Win32Exception::FormatErrorMessage(DWORD dwWin32Error)
    {
        std::wstring msg;
        DWORD SystemLocale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        HLOCAL hmem;
        BOOL i = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, dwWin32Error, SystemLocale, (LPTSTR)&hmem, 0, NULL
        );
        msg = (LPTSTR)LocalLock(hmem);
        LocalFree(hmem);
        return msg;
    }
}
