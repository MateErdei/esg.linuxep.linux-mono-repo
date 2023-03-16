// Copyright 2023 Sophos Limited. All rights reserved.

#include "hash.h"

#include "crypto_utils.h"
#include "hash_exception.h"
#include "libcrypto-compat.h"
#include "libcryptosupport.h"

#include <cassert>
#include <iostream>

_Success_(return) bool crypto::parse_hash_algorithm(const std::string &name, _Out_ crypto::hash_algo &algo)
{
    if (name == "md5")
        algo = crypto::ALGO_MD5;
    else if (name == "sha1")
        algo = crypto::ALGO_SHA1;
    else if (name == "sha256")
        algo = crypto::ALGO_SHA256;
    else if (name == "sha384")
        algo = crypto::ALGO_SHA384;
    else if (name == "sha512")
        algo = crypto::ALGO_SHA512;
    else
        return false;
    return true;
}

using hash_t = crypto::hash::hash_t;

namespace
{
    class one_hash
    {
    public:
        one_hash(int algorithms, crypto::hash_algo algo)
        {
            enabled_ = (algorithms & algo) != 0;
            if (!enabled_)
            {
                return;
            }
            algorithm_ = construct_digest_algorithm(algo);
            hash_ = EVP_MD_CTX_new();
            EVP_DigestInit(hash_, algorithm_);
        }

        ~one_hash()
        {
            if (hash_)
            {
                EVP_MD_CTX_free(hash_);
            }
        }

        void add_data(const char * p, size_t n)
        {
            if (!enabled_)
            {
                return;
            }
            EVP_DigestUpdate(hash_, p, n);
        }

        std::string rawdigest()
        {
            assert(enabled_);
            if (!enabled_)
            {
                return "";
            }

            if (!rawdigest_.empty())
            {
                return rawdigest_;
            }

            unsigned char hash_buf[EVP_MAX_MD_SIZE];
            unsigned int hash_len = 0;
            EVP_DigestFinal(hash_, hash_buf, &hash_len);


            rawdigest_ = std::string((char*)hash_buf, hash_len);
            return rawdigest_;
        }

        hash_t hexdigest()
        {
            assert(enabled_);
            if (!enabled_)
            {
                return "";
            }
            auto raw = rawdigest();
            return crypto::hex(raw);
        }

        bool enabled_ = false;
    private:
        const EVP_MD* algorithm_ = nullptr;
        EVP_MD_CTX* hash_ = nullptr;
        std::string rawdigest_;
    };
} // namespace

class crypto::hash::hash_impl
{
public:
    explicit hash_impl(int algorithms)
        : md5_(algorithms, crypto::hash_algo::ALGO_MD5)
        , sha1_(algorithms, crypto::hash_algo::ALGO_SHA1)
        , sha256_(algorithms, crypto::hash_algo::ALGO_SHA256)
        , sha384_(algorithms, crypto::hash_algo::ALGO_SHA384)
    {
    }

    one_hash md5_;
    one_hash sha1_;
    one_hash sha256_;
    one_hash sha384_;


    void add_data(const char * p, size_t n)
    {
        md5_.add_data(p, n);
        sha1_.add_data(p, n);
        sha256_.add_data(p, n);
        sha384_.add_data(p, n);
    }

    one_hash& get_one_hash(crypto::hash_algo algo)
    {
        switch (algo)
        {
            case crypto::hash_algo::ALGO_MD5:
                return md5_;
            case crypto::hash_algo::ALGO_SHA1:
                return sha1_;
            case crypto::hash_algo::ALGO_SHA256:
                return sha256_;
            case crypto::hash_algo::ALGO_SHA384:
                return sha384_;
            default:
                // not reached unless we forget to add a case!
                throw crypto::hash_exception("unknown algorithm: " + std::to_string(algo));
        }
    }
};

bool crypto::hash::is_algorithm_enabled(hash_algo algo) const
{
    return impl_->get_one_hash(algo).enabled_;
}

static void validate_algorithm_enabled(bool enabled)
{
    if (!enabled)
        throw crypto::hash_exception("invalid algorithm: not enabled in constructor");
}

crypto::hash::hash(int algorithms)
    : impl_{ std::make_unique<hash_impl>(algorithms) }
    , bytes_{ 0 }
{
}

void crypto::hash::add_data(const char * p, size_t n) const
{
    bytes_ += n;
    impl_->add_data(p, n);
}

static void copy_stream(const crypto::hash& self, std::istream &in, std::ostream *out)
{
    while (in)
    {
        char buff[8192];
        in.read(buff, std::streamsize(sizeof buff));
        auto buffLen = in.gcount();
        if (buffLen)
        {
            if (out)
                out->write(buff, buffLen);
            self.add_data(buff, (int)buffLen);
        }
    }
}

void crypto::hash::add_stream(std::istream &str) const
{
    ::copy_stream(*this, str, nullptr);
}

void crypto::hash::copy_stream(std::istream & in, std::ostream & out) const
{
    ::copy_stream(*this, in, &out);
}

void crypto::hash::assert_eq(crypto::hash_algo algo, const hash_t& against) const
{
    if (!is_eq(algo, against))
    {
        throw crypto::hash_exception(against);
    }
}

bool crypto::hash::is_eq(hash_algo algo, const hash_t& against) const
{
    return digest(algo) == against;
}

size_t crypto::hash::bytes() const
{
    return bytes_;
}

std::string crypto::hash::rawdigest(crypto::hash_algo algo) const
{
    validate_algorithm_enabled(is_algorithm_enabled(algo));
    return impl_->get_one_hash(algo).rawdigest();
}

hash_t crypto::hash::digest(crypto::hash_algo algo) const
{
    validate_algorithm_enabled(is_algorithm_enabled(algo));
    return impl_->get_one_hash(algo).hexdigest();
}
