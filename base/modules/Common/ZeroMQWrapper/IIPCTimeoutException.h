//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_IIPCTIMEOUTEXCEPTION_H
#define EVEREST_BASE_IIPCTIMEOUTEXCEPTION_H


#include "IIPCException.h"

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCTimeoutException : public IIPCException
        {
        public:
            explicit IIPCTimeoutException(const std::string& what)
                    : IIPCException(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_IIPCTIMEOUTEXCEPTION_H
