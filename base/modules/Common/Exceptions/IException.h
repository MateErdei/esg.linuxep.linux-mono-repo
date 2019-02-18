/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <exception>
#include <stdexcept>

namespace Common
{
    namespace Exceptions
    {
        class IException : public std::runtime_error
        {
        public:
            virtual ~IException() = default;
            explicit IException(const std::string& what) : std::runtime_error(what) {}
        };
    } // namespace Exceptions
} // namespace Common
