//
// Created by pair on 11/06/18.
//

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
            explicit IException(const std::string& what)
                    : std::runtime_error(what)
            {}
        };
    }
}


