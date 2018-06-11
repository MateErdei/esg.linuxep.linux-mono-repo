//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_IEXCEPTION_H
#define EVEREST_BASE_IEXCEPTION_H

#include <exception>
#include <stdexcept>

namespace Common
{
    namespace Exceptions
    {
        class IException : public std::runtime_error
        {
        public:
            explicit IException(const std::string& what)
                    : std::runtime_error(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_IEXCEPTION_H
