/*-------------------------------------------------------------------------*\

crypto::hash_exceptions are thrown when a precalculated hash doesn't agree
with the thing actually hashed.

\*-------------------------------------------------------------------------*/

#pragma once

#include <stdexcept>

namespace crypto
{
    class hash_exception : public std::runtime_error
    {
    public:
        explicit hash_exception(const std::string &name)
            : std::runtime_error("Checksum error: " + name)
        {}
    };
}
