//
// Created by pair on 08/06/18.
//

#ifndef EVEREST_BASE_IIPCEXCEPTION_H
#define EVEREST_BASE_IIPCEXCEPTION_H

#include <exception>
#include <stdexcept>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCException : public std::runtime_error
        {
        public:
            explicit IIPCException(const std::string& what)
                    : std::runtime_error(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_IIPCEXCEPTION_H
