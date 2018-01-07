#include "stdafx.h"
#include "hxc.h"
#include <cryptdlg.h>

#pragma comment(lib, "Crypt32.lib")

#define CHECK_RETURN(fun, exp)  \
    if (fun){  \
        DWORD dw = GetLastError();  \
        exp e(dw);  \
        SET_EXCEPTION(e);  \
        throw e;  \
    }

namespace hxc
{
    CryptographicException::CryptographicException(DWORD err) : Exception(err)
    {
    }

    CryptographicException::CryptographicException(const std::wstring & message) : Exception(message)
    {
    }

    X509Certificate::X509Certificate(const std::vector<BYTE>& cert)
    {
        if (!::CryptAcquireContext(&m_hProv, _T("hxc"), MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
        {
            ::CryptAcquireContext(&m_hProv, _T("hxc"), MS_ENHANCED_PROV, PROV_RSA_FULL,
                CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET);
        }
        //CertCreateContext
    }

#pragma region Aes
    Aes::Aes() : _BlockSize(128), _Mode(CRYPT_MODE_CBC), _Padding(PKCS5_PADDING),
        m_hKey(NULL), _KeySize(128), _IV(_BlockSize / 8, (BYTE)0)
    {
        _LegalBlockSizes.push_back(_BlockSize);
        for (int i = 128; i <= 256; i += 64)
        {
            _LegalKeySizes.push_back(i);
        }

        CHECK_RETURN(!CryptAcquireContextW(&m_hProv, nullptr, MS_ENH_RSA_AES_PROV_W, PROV_RSA_AES, CRYPT_SILENT), CryptographicException);
    }

    Aes::~Aes()
    {
        if (m_hKey != NULL)
        {
            CryptDestroyKey(m_hKey);
        }

        if (m_hProv != NULL)
        {
            CryptReleaseContext(m_hProv, 0);
        }
    }

    void Aes::GenerateIV()
    {
        int iv_len = get__LegalBlockSizes()[0] / 8;
        std::vector<BYTE> iv(iv_len);
        CHECK_RETURN(!::CryptGenRandom(m_hProv, iv_len, &iv[0]), CryptographicException);

        set__IV(iv);
    }

    void Aes::GenerateKey()
    {
        int key_len = get__LegalKeySizes()[0] / 8;
        std::vector<BYTE> key(key_len);
        CHECK_RETURN(!::CryptGenRandom(m_hProv, key_len, &key[0]), CryptographicException);

        set__Key(key);
    }

    int Aes::EncryptBlock(
        const std::vector<BYTE>& inputBuffer,
        int inputOffset,
        int inputCount,
        bool isFinalBlock,
        std::vector<BYTE>& outputBuffer
    )
    {
        DWORD dwDataLen = inputCount;
        CHECK_RETURN(!CryptEncrypt(m_hKey, NULL, (BOOL)isFinalBlock, 0, nullptr, &dwDataLen, 0), CryptographicException);
        outputBuffer.resize(dwDataLen);
        dwDataLen = inputCount;
        memcpy(&outputBuffer[0], &inputBuffer[inputOffset], inputCount);
        CHECK_RETURN(!CryptEncrypt(m_hKey, NULL, (BOOL)isFinalBlock, 0, &outputBuffer[0], &dwDataLen, outputBuffer.size()), CryptographicException);
        return dwDataLen;
    }

    int Aes::DecryptBlock(
        std::vector<BYTE>& inputBuffer,
        int inputOffset,
        int inputCount,
        bool isFinalBlock
    )
    {
        DWORD dwDataLen = inputCount;
        CHECK_RETURN(!CryptDecrypt(m_hKey, NULL, (BOOL)isFinalBlock, 0, &inputBuffer[0], &dwDataLen), CryptographicException);
        return dwDataLen;
    }

    int Aes::get_BlockSize() const
    {
        return _BlockSize;
    }

    void Aes::set__BlockSize(int value)
    {
        _IV.assign(value / 8, (BYTE)0);
        _BlockSize = value;
    }

    std::vector<BYTE> Aes::get__IV() const
    {
        return _IV;
    }

    void Aes::set__IV(const std::vector<BYTE>& value)
    {
        int SizeIsValid = false;
        for (auto i = _LegalBlockSizes.begin(); i != _LegalBlockSizes.end(); i++)
        {
            if (*i == value.size() * 8)
            {
                SizeIsValid = true;
                break;
            }
        }
        if (!SizeIsValid)
        {
            CryptographicException e(L"非法的初始化向量长度。");
            SET_EXCEPTION(e);
            throw e;
        }

        if (m_hKey != NULL)
        {
            CHECK_RETURN(!CryptSetKeyParam(m_hKey, KP_IV, &_IV[0], 0), CryptographicException);
        }
        _IV = value;
    }

    std::vector<BYTE> Aes::get__Key() const
    {
        return _Key;
    }

    void Aes::set__Key(const std::vector<BYTE>& value)
    {
        set__KeySize(value.size() * 8);
        _Key = value;

        struct _keyParam
        {
            BLOBHEADER head;
            int keyLen;
            BYTE keyvalue[256];
        };

        int len = sizeof(BLOBHEADER) + sizeof(int) + value.size();
        _keyParam KeyParam = {};
        KeyParam.head.bType = PLAINTEXTKEYBLOB;
        KeyParam.head.bVersion = CUR_BLOB_VERSION;
        KeyParam.head.aiKeyAlg = CALG_AES_128;
        if (get__KeySize() == 192)
            KeyParam.head.aiKeyAlg = CALG_AES_192;
        else if (get__KeySize() == 256)
            KeyParam.head.aiKeyAlg = CALG_AES_256;
        KeyParam.keyLen = value.size();
        memcpy_s(KeyParam.keyvalue, 256, &value[0], value.size());
        CHECK_RETURN(!CryptImportKey(m_hProv, (LPBYTE)&KeyParam, len, NULL, CRYPT_EXPORTABLE, &m_hKey), CryptographicException);
        std::vector<BYTE> iv = _IV;
        set__IV(iv);
        set__Mode(_Mode);
        set__Padding(_Padding);
    }

    int Aes::get__KeySize() const
    {
        return _KeySize;
    }

    void Aes::set__KeySize(int value)
    {
        bool SizeIsValid = false;
        for (auto i = _LegalKeySizes.begin(); i != _LegalKeySizes.end(); i++)
        {
            if (*i == value)
            {
                SizeIsValid = true;
                break;
            }
        }
        if (!SizeIsValid)
        {
            CryptographicException e(L"非法的密钥长度。");
            SET_EXCEPTION(e);
            throw e;
        }
        if (m_hKey != NULL)
        {
            CHECK_RETURN(!CryptDestroyKey(m_hKey), CryptographicException);
            m_hKey = NULL;
        }
        _KeySize = value;
    }

    const std::vector<int>& Aes::get__LegalBlockSizes() const
    {
        return _LegalBlockSizes;
    }

    const std::vector<int>& Aes::get__LegalKeySizes() const
    {
        return _LegalKeySizes;
    }

    int Aes::get__Mode() const
    {
        return _Mode;
    }

    void Aes::set__Mode(int value)
    {
        CHECK_RETURN(!CryptSetKeyParam(m_hKey, KP_MODE, (BYTE*)&value, 0), CryptographicException);
        _Mode = value;
    }

    int Aes::get_Padding() const
    {
        return _Padding;
    }

    void Aes::set__Padding(int value)
    {
        CHECK_RETURN(!CryptSetKeyParam(m_hKey, KP_PADDING, (BYTE*)&value, 0), CryptographicException);
        _Padding = value;
    }
#pragma endregion
} // namespace hxc
