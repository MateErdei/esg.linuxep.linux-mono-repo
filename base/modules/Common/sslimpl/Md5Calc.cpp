/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Md5Calc.h"

#include <openssl/evp.h>

#include <iomanip>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cassert>

namespace Common
{
    namespace sslimpl
    {
        std::string md5(const std::string& input)
        {
            const EVP_MD* evp = EVP_md5();

            if (evp == 0)
            {
                throw std::runtime_error("EVP_md5() returned NULL");
            }

            EVP_MD_CTX* ctx = EVP_MD_CTX_create();

            if (ctx == nullptr)
            {
                throw std::runtime_error("EVP_MD_CTX_create() returned NULL");
            }

            int ret = EVP_DigestInit(ctx, evp);
            assert(ret == 1);
            static_cast<void>(ret);

            ret = EVP_DigestUpdate(ctx, input.data(), input.size());
            assert(ret == 1);


            std::vector<unsigned char> buffer(EVP_MAX_MD_SIZE);
            unsigned int len = 0;
            ret = EVP_DigestFinal(ctx, &buffer[0], &len);
            assert(ret == 1);
            EVP_MD_CTX_destroy(ctx);

            std::ostringstream stream;
            buffer.resize(len);
            stream << std::hex << std::setfill('0') << std::nouppercase;

            for (std::vector<unsigned char>::const_iterator it = buffer.begin(); it != buffer.end(); ++it)
            {
                stream << std::setw(2) << int(*it);
            }

            return stream.str();
        }
    } // namespace sslimpl
} // namespace Common
