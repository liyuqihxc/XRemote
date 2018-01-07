#include "stdafx.h"
#include "hxc.h"
#include "../Res.h"

using namespace std;

namespace hxc
{
    void Utility::ToBase64String(std::string & base64, const std::vector<BYTE>& src)
    {
        //table
        static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

        //check 
        if (src.empty())
            return;

        int num = src.size() / 3 * 4;
        num += ((src.size() % 3 != 0) ? 4 : 0);
        if (num == 0)
        {
            return;
        }
        base64.resize(num);

        num = src.size() % 3;
        int num2 = src.size() - num;
        int num3 = 0;
        int num4 = 0;
        const char* ptr = table;
        int i;
        for (i = 0; i < num2; i += 3)
        {
            base64[num3] = *(ptr + ((*(src.data() + i / 1) & 252) >> 2));
            base64[(num3 + 1)] = *(ptr + ((int)(*(src.data() + i / 1) & 3) << 4 | (*(src.data() + (i + 1) / 1) & 240) >> 4));
            base64[(num3 + 2)] = *(ptr + ((int)(*(src.data() + (i + 1) / 1) & 15) << 2 | (*(src.data() + (i + 2) / 1) & 192) >> 6));
            base64[(num3 + 3)] = *(ptr + (*(src.data() + (i + 2) / 1) & 63));
            num3 += 4;
        }
        i = num2;
        if (num != 1)
        {
            if (num == 2)
            {
                base64[num3] = *(ptr + ((*(src.data() + i / 1) & 252) >> 2));
                base64[(num3 + 1)] = *(ptr + ((int)(*(src.data() + i / 1) & 3) << 4 | (*(src.data() + (i + 1) / 1) & 240) >> 4));
                base64[(num3 + 2)] = *(ptr + ((*(src.data() + (i + 1) / 1) & 15) << 2));
                base64[(num3 + 3)] = *(ptr + 64);
                num3 += 4;
            }
        }
        else
        {
            base64[num3] = *(ptr + ((*(src.data() + i / 1) & 252) >> 2));
            base64[(num3 + 1)] = *(ptr + ((*(src.data() + i / 1) & 3) << 4));
            base64[(num3 + 2)] = *(ptr + 64);
            base64[(num3 + 3)] = *(ptr + 64);
            num3 += 4;
        }

        //ok
        return;
    }

    void Utility::FromBase64String(const std::string & base64, std::vector<BYTE>& binvalue)
    {
        //check
        if (base64.empty())
            return;

        int inputLength = base64.size();
        for (; inputLength > 0; inputLength--)
        {
            int num = *(base64.begin() + (inputLength - 1));
            if (num != 32 && num != 10 && num != 13 && num != 9)
                break;
        }
        binvalue.resize(FromBase64_ComputeResultLength(base64, inputLength));

        const char* ptr = base64.c_str();
        LPBYTE ptr2 = binvalue.data();
        const char* ptr3 = ptr + inputLength;
        LPBYTE ptr4 = ptr2 + binvalue.size() / 1;
        unsigned int num = 255u;
        while (ptr < ptr3)
        {
            unsigned int num2 = *ptr;
            ptr++;
            if (num2 - 65u <= 25u)
            {
                num2 -= 65u;
            }
            else
            {
                if (num2 - 97u <= 25u)
                {
                    num2 -= 71u;
                }
                else
                {
                    if (num2 - 48u > 9u)
                    {
                        if (num2 <= 32u)
                        {
                            switch (num2)
                            {
                            case 9u:
                            case 10u:
                            case 13u:
                                continue;
                            case 11u:
                            case 12u:
                                break;
                            default:
                                if (num2 == 32u)
                                {
                                    continue;
                                }
                                break;
                            }
                        }
                        else
                        {
                            if (num2 == 43u)
                            {
                                num2 = 62u;
                                goto IL_BA;
                            }
                            if (num2 == 47u)
                            {
                                num2 = 63u;
                                goto IL_BA;
                            }
                            if (num2 == 61u)
                            {
                                if (ptr == ptr3)
                                {
                                    num <<= 6;
                                    if ((num & 2147483648u) == 0u)
                                    {
                                        throw FormatException(L"BadBase64CharArrayLength");
                                    }
                                    if ((ptr4 - (LPBYTE)((size_t)ptr2 / 1)) / 1 < 2)
                                    {
                                        return;
                                    }
                                    *(ptr2++) = (byte)(num >> 16);
                                    *(ptr2++) = (byte)(num >> 8);
                                    num = 255u;
                                    break;
                                }
                                else
                                {
                                    while (ptr < ptr3 - 1)
                                    {
                                        int num3 = *ptr;
                                        if (num3 != 32 && num3 != 10 && num3 != 13 && num3 != 9)
                                        {
                                            break;
                                        }
                                        ptr++;
                                    }
                                    if (ptr != ptr3 - 1 || *ptr != 61)
                                    {
                                        throw FormatException(L"BadBase64Char");
                                    }
                                    num <<= 12;
                                    if ((num & 2147483648u) == 0u)
                                    {
                                        throw FormatException(L"BadBase64CharArrayLength");
                                    }
                                    if ((ptr4 - (LPBYTE)((size_t)ptr2 / 1)) / 1 < 1)
                                    {
                                        return;
                                    }
                                    *(ptr2++) = (byte)(num >> 16);
                                    num = 255u;
                                    break;
                                }
                            }
                        }
                        throw FormatException(L"BadBase64Char");
                    }
                    num2 -= 4294967292u;
                }
            }
        IL_BA:
            num = (num << 6 | num2);
            if ((num & 2147483648u) != 0u)
            {
                if ((ptr4 - (LPBYTE)((size_t)ptr2 / 1)) / 1 < 3)
                {
                    return;
                }
                *ptr2 = (byte)(num >> 16);
                ptr2[1] = (byte)(num >> 8);
                ptr2[2 / 1] = (byte)num;
                ptr2 += 3 / 1;
                num = 255u;
            }
        }
        if (num != 255u)
        {
            throw FormatException(L"Format_BadBase64CharArrayLength");
        }
        
        return;
    }

    void Utility::Unicode2Utf8(const wstring & src, string & utf8)
    {
        int iLen = WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, nullptr, 0, nullptr, nullptr);
        LPSTR str = new CHAR[iLen];
        ZeroMemory(str, sizeof(CHAR) * iLen);
        WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, str, iLen, nullptr, nullptr);
        utf8 = str;
        delete[] str;
    }

    void Utility::Utf82Unicode(const string & src, wstring & unicode)
    {
        int iLen = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, nullptr, 0);
        LPWSTR str = new WCHAR[iLen];
        ZeroMemory(str, sizeof(WCHAR) * iLen);
        MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, str, iLen);
        unicode = str;
        delete[] str;
    }

    const vector<BYTE> Utility::get__PasswordAsKey(void)
    {
        wstring password = Resource::get__Password();
        if (password.size() >= 16)
        {
            Exception e(L"资源文件当中的Password值字符长度超过16个字符。");
            SET_EXCEPTION(e);
            throw e;
        }
        string utf8;
        Utility::Unicode2Utf8(password, utf8);
        BYTE key[16] = {};
        memcpy_s(key, 16, utf8.c_str(), (utf8.size() + 1) * sizeof(CHAR));
        return vector<BYTE>(key, key + 16);
    }

    int Utility::FromBase64_ComputeResultLength(const std::string & base64, int inputLength)
    {
        const char* inputPtr = base64.c_str();
        const char* ptr = inputPtr + inputLength;
        int num = inputLength;
        int num2 = 0;
        while (inputPtr < ptr)
        {
            unsigned int num3 = inputPtr[0];
            inputPtr++;
            if (num3 <= 32u)
            {
                num--;
            }
            else
            {
                if (num3 == 61u)
                {
                    num--;
                    num2++;
                }
            }
        }
        if (num2 != 0)
        {
            if (num2 == 1)
            {
                num2 = 2;
            }
            else
            {
                if (num2 != 2)
                {
                    FormatException e(L"BadBase64Char");
                    SET_EXCEPTION(e);
                    throw e;
                }
                num2 = 1;
            }
        }
        return num / 4 * 3 + num2;
    }
}
