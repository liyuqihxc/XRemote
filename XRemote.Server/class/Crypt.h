#pragma once

#if !defined _CERTIFICATE_H_
#define _CERTIFICATE_H_

#include <WinCrypt.h>

namespace hxc
{
    class CryptographicException : public Exception
    {
    public:
        CryptographicException(DWORD err);
        CryptographicException(const std::wstring& message);
    };

    class X509Certificate
    {
    public:
        explicit X509Certificate(const std::vector<BYTE>& cert);
        explicit X509Certificate(PCERT_CONTEXT pCertCtx);
    public:
        const std::wstring get__Issuer();
        const std::vector<BYTE*>& get__PublicKey();
        void get__PrivateKey();
        const std::vector<BYTE> get__RawData();
    private:
        HCRYPTPROV m_hProv;
    };

    class Aes
    {
    public:
        Aes();
        ~Aes();
    public:
        void GenerateIV();
        void GenerateKey();
        int EncryptBlock(
            const std::vector<BYTE>& inputBuffer,
            int inputOffset,
            int inputCount,
            bool isFinalBlock,
            std::vector<BYTE>& outputBuffer
        );

        int DecryptBlock(
            std::vector<BYTE>& inputBuffer,
            int inputOffset,
            int inputCount,
            bool isFinalBlock
        );

    public:
        int get_BlockSize(void) const;
        void set__BlockSize(int value);

        std::vector<BYTE> get__IV(void) const;
        void set__IV(const std::vector<BYTE>& value);

        std::vector<BYTE> get__Key(void) const;
        void set__Key(const std::vector<BYTE>& value);

        int get__KeySize(void) const;
        void set__KeySize(int value);

        const std::vector<int>& get__LegalBlockSizes(void) const;

        const std::vector<int>& get__LegalKeySizes(void) const;

        int get__Mode(void) const;
        void set__Mode(int value);

        int get_Padding(void) const;
        void set__Padding(int value);

    private:
        int _BlockSize;
        std::vector<BYTE> _IV;
        std::vector<BYTE> _Key;
        std::vector<int> _LegalBlockSizes;
        std::vector<int> _LegalKeySizes;
        int _Mode;
        int _Padding;
        int _KeySize;
        HCRYPTPROV m_hProv;
        HCRYPTKEY m_hKey;
    };

};//namespace hxc

#endif//if !defined _CERTIFICATE_H_
