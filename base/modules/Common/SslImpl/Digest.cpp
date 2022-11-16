// Copyright 2018-2022, Sophos Limited. All rights reserved.

#include "Digest.h"

#include <openssl/evp.h>

#include <iomanip>
#include <vector>

namespace Common::SslImpl
{
    namespace
    {
        // RAII EVP_MD_CTX wrapper
        class MdContext
        {
        public:
            MdContext() : mdctx_(EVP_MD_CTX_new())
            {
                if (mdctx_ == nullptr)
                {
                    throw std::runtime_error("EVP_MD_CTX_new failed");
                }
            }

            ~MdContext()
            {
                EVP_MD_CTX_free(mdctx_); // Safe to call with nullptr
                mdctx_ = nullptr;
            }

            MdContext(const MdContext&) = delete;
            MdContext& operator=(const MdContext& other) = delete;
            MdContext(MdContext&& other) noexcept
            {
                mdctx_ = other.mdctx_;
                other.mdctx_ = nullptr;
            }
            MdContext& operator=(MdContext&& other) noexcept
            {
                mdctx_ = other.mdctx_;
                other.mdctx_ = nullptr;
                return *this;
            }

            [[nodiscard]] EVP_MD_CTX* get() { return mdctx_; }

        private:
            EVP_MD_CTX* mdctx_;
        };
    } // namespace

    std::string calculateDigest(Digest digestName, std::istream& inStream)
    {
        if (!inStream.good())
        {
            throw std::runtime_error("Provided istream is not in a good state for reading");
        }

        const char* digestNameString;
        switch (digestName)
        {
            case Digest::md5:
                digestNameString = "md5";
                break;
            case Digest::sha256:
                digestNameString = "sha256";
                break;
            case Digest::sha1:
                digestNameString = "sha1";
                break;
        }

        // TODO: replace with EVP_MD_fetch for OpenSSL 3.0
        const EVP_MD* md = EVP_get_digestbyname(digestNameString);
        if (md == nullptr)
        {
            throw std::runtime_error(std::string("Unknown message digest ") + digestNameString);
        }

        MdContext mdContext;

        if (EVP_DigestInit_ex(mdContext.get(), md, nullptr) != 1)
        {
            throw std::runtime_error("EVP_DigestInit_ex failed");
        }

        while (inStream.good())
        {
            char buffer[digestBufferSize];
            inStream.read(buffer, digestBufferSize);
            if (EVP_DigestUpdate(mdContext.get(), buffer, inStream.gcount()) != 1)
            {
                throw std::runtime_error("EVP_DigestUpdate failed");
            }
        }

        if (inStream.bad())
        {
            throw std::runtime_error("Reading ended with a bad istream state");
        }

        std::vector<unsigned char> digest(EVP_MAX_MD_SIZE);
        unsigned int len = 0;

        if (EVP_DigestFinal_ex(mdContext.get(), digest.data(), &len) != 1)
        {
            throw std::runtime_error("EVP_DigestFinal_ex failed");
        }

        digest.resize(len);
        std::ostringstream stream;
        stream << std::hex << std::setfill('0') << std::nouppercase;

        for (unsigned char byte : digest)
        {
            stream << std::setw(2) << int(byte);
        }

        return stream.str();
    }

    std::string calculateDigest(Digest digestName, const std::string& input)
    {
        std::istringstream istream(input);
        return calculateDigest(digestName, istream);
    }
} // namespace Common::SslImpl
