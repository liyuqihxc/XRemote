#pragma once

#if !defined _EXCEPTIONS
#define _EXCEPTIONS

namespace hxc
{
#define SET_EXCEPTION(e) e.set__SourceFileName(__FILE__);e.set__LineNumber(__LINE__);e.set__Function(__FUNCTION__);

    class Exception : public std::exception
    {
    public:
        Exception();
        Exception(const Exception& e);
        Exception& operator=(const Exception& e);
        explicit Exception(HRESULT hresult);
        explicit Exception(const std::wstring& Message);
    public:
        HRESULT get__HResult() const;
        void set__HResult(HRESULT value);

        const std::wstring& get__Message() const;
        void set__Message(const std::wstring& value);

        const std::wstring& get__SourceFileName() const;
        void set__SourceFileName(const std::wstring& value);
        void set__SourceFileName(const std::string& value);

        const std::wstring& get__Function() const;
        void set__Function(const std::wstring& value);
        void set__Function(const std::string& value);

        int get__LineNumber() const;
        void set__LineNumber(int value);

        virtual char const* what() const;

        static DWORD NTSTATUS_To_Win32Err(LONG code);
        //void SaveDumpFile();
    protected:
        HRESULT _HResult;
        std::wstring _MessageW;
        std::string _MessageA;
        std::wstring _SourceFileName;
        std::wstring _Function;
        int _LineNumber;
    private:
        static HMODULE hNtdll;
        static std::function<ULONG _stdcall(LONG)> _NTSTATUS_To_Win32Err;
    };

    class MemoryException : public Exception
    {
    public:
        MemoryException(const std::wstring& Message);
        explicit MemoryException(const std::bad_alloc& innerException);
    };

    class NotImplementedException : public Exception
    {
    public:
        NotImplementedException();
    };

    class ArgumentException : public Exception
    {
    public:
        ArgumentException();
    };

    class ArgumentOutOfRangeException : public ArgumentException
    {

    };

    class FormatException : public Exception
    {
    public:
        FormatException();
        FormatException(const std::wstring& Message);
    };

    class InvalidOperationException : public Exception
    {
    public:
        //InvalidOperationException() {}
    };

    class NotSupportedException : public Exception
    {
    public:
    };

    class NullReferenceException : public Exception
    {
    public:
        //NullReferenceException() {}
    };

    class Win32Exception : public Exception
    {
    public:
        explicit Win32Exception(DWORD dwWin32Error);
        static std::wstring FormatErrorMessage(DWORD dwWin32Error);
    };

};//namespace hxc

#endif//!defined _EXCEPTIONS