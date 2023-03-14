// Copyright 2023 Sophos Limited. All rights reserved.

#ifndef VERSIG_HASH_H
#define VERSIG_HASH_H

#include "hash_algorithm.h"

#include <memory>
#include <string>

#ifndef _WIN32
#define _Success_(...)
#define _Out_
#else
#include <Windows.h>
#endif

namespace crypto
{
    _Success_(return) bool parse_hash_algorithm(const std::string& name, _Out_ hash_algo &algo);

    class hash final
    {
    public:
        using hash_t = std::string;
        explicit hash(int algorithms);

        virtual void add_data(const char *p, size_t n) const;
        virtual void add_stream(std::istream &) const;
        virtual void copy_stream(std::istream &in, std::ostream &out) const;

        template<typename T>
        void add_data(const std::basic_string<T> &str) const
        {
            const char *p = reinterpret_cast<const char *>(str.c_str());
            int n = int(str.length() * sizeof(T));
            add_data(p, n);
        }

        // Returns true if this object implements the given algorithm.
        // Returns false if either the algorithm was not provided to the constructor, or
        // if the algorithm is not implemented on this operating system.
        bool is_algorithm_enabled(hash_algo algo) const;

        // Return the digest as a hex string.
        // Throws crypto::hash_exception if algo wasn't passed to the constructor
        hash_t digest(hash_algo algo) const;

        // Returns the digest as a binary blob.
        // Throws crypto::hash_exception if algo wasn't passed to the constructor
        std::string rawdigest(hash_algo algo) const;

        // Asserts the computed hash is equal to 'expected'.
        // Throws crypto::hash_exception if the hash is incorrect
        // Throws crypto::hash_exception if algo wasn't passed to the constructor
        void assert_eq(hash_algo algo, const hash_t& expected) const;

        // Compares the hash to 'expected'.
        // Throws crypto::hash_exception if algo wasn't passed to the constructor
        bool is_eq(hash_algo algo, const hash_t& expected) const;

        // Return the number of data bytes passed to add_data()/copy_stream()/add_stream().
        size_t bytes() const;

        // static convenience method to compute a digest from raw string data
        template<typename T>
        static hash_t digest(hash_algo algo, const std::basic_string<T> &str)
        {
            const char *p = reinterpret_cast<const char *>(str.c_str());
            int n = int(str.length() * sizeof(T));

            hash h(algo);
            h.add_data(p, n);
            return h.digest(algo);
        }

    private:
        class hash_impl;

        std::unique_ptr<hash_impl> impl_;
        mutable size_t bytes_;
    };
}

#endif // VERSIG_HASH_H
