#pragma once

#ifndef _UTILITY_H_
#define _UTILITY_H_
namespace hxc
{
    class Utility
    {
    private:
        Utility(){}
    public:
        static void ToBase64String(std::string& base64, const std::vector<BYTE>& src);
        static void FromBase64String(const std::string& base64, std::vector<BYTE>& binvalue);
        static void Unicode2Utf8(const std::wstring& src, std::string& utf8);
        static void Utf82Unicode(const std::string& src, std::wstring& unicode);
        static const std::vector<BYTE> get__PasswordAsKey(std::wstring password);
    private:
        static int FromBase64_ComputeResultLength(const std::string& base64, int inputLength);
    };
}//namespace hxc
#endif //ifndef _UTILITY_H_
