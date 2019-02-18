/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Obscurity.h"

#include "Cipher.h"
#include "KeyDo.h"
#include "KeyFa.h"
#include "KeyMi.h"
#include "KeyRa.h"
#include "Logger.h"

#include "Common/Obfuscation/IObscurityException.h"

#include <cstring>
#include <iostream>
#include <sstream>

namespace
{
    void Scribble(unsigned char* data, size_t dataLen)
    {
        // it is necessary to make the pointer volatile to prevent the compiler to
        // remove this code ( it would be better to use memset_s, but it is not available)
        volatile unsigned char* p = data;
        int size_to_remove = dataLen;
        while (size_to_remove--)
        {
            *p++ = 'x';
        }
    }

} // namespace

namespace Common
{
    namespace ObfuscationImpl
    {
        /* Notes:
         * First of all, this implementation is an attempt to obscure sensitive data.
         *
         *
         *
         *
         * The goal of this implementation is to make 'raise the bar' of difficulty in
         * discovering sensitive data. The techniques it uses to do this are:
         * 1) uses an asymetric key to encrypt and decrypt the data
         *    However, the asymetric key is stored in the program (i.e. hardcoded)
         * 2) So, the asymetric key is split into parts, and
         * 3) Each part is obscured in a different way.
         * 4) The assembled asymetric key is kept in memory for the minimum period.
         *
         * However, the algorithms to obscure the key **very** simplistic;
         *    DO NOT RELY ON THEM TO SECURE SENSITIVE DATA!!!!
         *
         * The simplest way to crack this is to catch the parameters as they are passed into OpenSSL
         * to perform the encrypt/decrypt processing, so the obscuring of the key at this level is a
         * compromise at deterring the stupid, knowing that the smart will look in the right place!
         *
         * In the event that we need a second obscurity algorithm, we mark (and check)
         * that this data was obscured by this algorithm by prepending a byte containing
         * this algorithm identifier.
         */
        // const unsigned char CObscurity::ALGORITHM_IDENT = 0x07;

        CObscurity::CObscurity() {}

        CObscurity::~CObscurity() {}

        Common::ObfuscationImpl::SecureDynamicBuffer CObscurity::GetPassword() const
        {
            constexpr int SECTION_COUNT = 4;
            LenVal_t sections[SECTION_COUNT];

            unsigned char* data1;
            size_t dataLen1;
            unsigned char* data2;
            size_t dataLen2;

            // Unobscure the sections of the password
            data1 = R1(CKeyDo::GetData(), CKeyDo::GetDataLen(), &dataLen1);
            data2 = R2(data1, dataLen1, &dataLen2);
            sections[0].value = data2;
            sections[0].len = dataLen2;
            delete[] data1;

            data1 = R3(CKeyRa::GetData(), CKeyRa::GetDataLen(), &dataLen1);
            data2 = R2(data1, dataLen1, &dataLen2);
            sections[1].value = data2;
            sections[1].len = dataLen2;
            delete[] data1;

            data1 = R3(CKeyMi::GetData(), CKeyMi::GetDataLen(), &dataLen1);
            data2 = R1(data1, dataLen1, &dataLen2);
            sections[2].value = data2;
            sections[2].len = dataLen2;
            delete[] data1;

            data1 = R2(CKeyFa::GetData(), CKeyFa::GetDataLen(), &dataLen1);
            data2 = R1(data1, dataLen1, &dataLen2);
            sections[3].value = data2;
            sections[3].len = dataLen2;
            delete[] data1;

            // Join up sections to make the password
            data1 = Join(sections, SECTION_COUNT, &dataLen1);

            SecureDynamicBuffer buffer(data1, data1 + dataLen1);

            // Free data
            Scribble(data1, dataLen1);
            delete[] data1;

            for (int i = 0; i < SECTION_COUNT; i++)
            {
                delete[] sections[i].value;
            }

            return buffer;
        }

        unsigned char* CObscurity::Join(const CObscurity::LenVal_t* sections, size_t count, size_t* dataLen)
        {
            // Join up sections - first count size of buffer
            size_t bufSize = 0;
            for (unsigned int i = 0; i < count; i++)
            {
                bufSize += sections[i].len;
            }

            // Now allocate and populate
            unsigned char* data = new unsigned char[bufSize];

            unsigned char* ptr = data;
            for (unsigned int j = 0; j < count; j++)
            {
                // ACE_OS::memcpy(ptr, sections[j].value, sections[j].len); //lint !e534 :ignore ret val
                memcpy(ptr, sections[j].value, sections[j].len); // lint !e534 :ignore ret val
                ptr += sections[j].len;
            }

            *dataLen = bufSize;
            return data;
        }

        unsigned char* CObscurity::R1(const unsigned char* obscuredData, size_t obscuredDataLen, size_t* dataLen)
        {
            // Reverse Transposition of data - ADCB becomes ABCD
            unsigned char* retData = new unsigned char[obscuredDataLen];
            for (unsigned int i = 0; i < obscuredDataLen; i++)
            {
                unsigned int mod2 = i % 2;
                if (0 == mod2)
                {
                    retData[i] = obscuredData[i / 2]; // even
                }
                else
                {
                    retData[i] = obscuredData[(obscuredDataLen - (i / 2)) - 1]; // odd
                }
            }
            *dataLen = obscuredDataLen;
            return retData;
        }

        static unsigned char GetMask(unsigned int salt)
        {
            static const unsigned char KEY[] = "FDGASSkwpodkfgfspwoegre;[addq[pad.col";
            unsigned char mask = 0;
            unsigned int mod3 = salt % 3;
            if (0 == mod3)
            {
                mask = KEY[salt * 13 % sizeof(KEY)];
            }
            else if (1 == mod3)
            {
                mask = KEY[salt * 11 % sizeof(KEY)];
            }
            else
            {
                mask = KEY[salt * 7 % sizeof(KEY)];
            }
            return mask;
        }

        unsigned char* CObscurity::R2(const unsigned char* obscuredData, size_t obscuredDataLen, size_t* dataLen)
        {
            // Reverse O2()
            unsigned char* retData = new unsigned char[obscuredDataLen];
            for (unsigned int i = 0; i < obscuredDataLen; i++)
            {
                retData[i] = obscuredData[i] ^ GetMask(i);
            }
            *dataLen = obscuredDataLen;
            return retData;
        }

        unsigned char* CObscurity::R3(const unsigned char* obscuredData, size_t obscuredDataLen, size_t* dataLen)
        {
            // Reverse data transposition
            unsigned char* tmpData = new unsigned char[(obscuredDataLen / 2) + 4];
            unsigned int tmpIndex = 0;
            for (unsigned int i = 0; i < obscuredDataLen; i++)
            {
                unsigned int mod8 = i % 8;
                if (0 == mod8)
                {
                    tmpData[tmpIndex++] = obscuredData[i];
                }
                else if (3 == mod8)
                {
                    tmpData[tmpIndex++] = obscuredData[i];
                }
                else if (5 == mod8)
                {
                    tmpData[tmpIndex++] = obscuredData[i];
                }
                else if (6 == mod8)
                {
                    tmpData[tmpIndex++] = obscuredData[i];
                }
            }
            *dataLen = tmpIndex;
            return tmpData;
        }

        SecureString CObscurity::Reveal(const std::string& obscuredData) const
        {
            constexpr int MarkAES256Algorithm = 8;
            SecureDynamicBuffer password = GetPassword();

            // Check algorithm identification
            if (MarkAES256Algorithm != obscuredData[0])
            {
                LOGDEBUG("Obscure:Invalid algorithm ident=" << static_cast<unsigned int>(obscuredData[0]));
                throw Common::Obfuscation::IObscurityException("SECDeobfuscation Failed.");
            }

            SecureDynamicBuffer obscuredText(begin(obscuredData) + 1, end(obscuredData));

            return Cipher::Decrypt(password, obscuredText);
        }
    } // namespace ObfuscationImpl
} // namespace Common
