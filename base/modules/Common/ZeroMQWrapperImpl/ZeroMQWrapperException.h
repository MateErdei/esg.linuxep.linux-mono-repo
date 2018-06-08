//
// Created by pair on 08/06/18.
//

#ifndef EVEREST_BASE_ZEROMQWRAPPEREXCEPTION_H
#define EVEREST_BASE_ZEROMQWRAPPEREXCEPTION_H


#include <Common/ZeroMQWrapper/IIPCException.h>

#include <string>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQWrapperException
            : public Common::ZeroMQWrapper::IIPCException
        {
        public:
            explicit ZeroMQWrapperException(const std::string &message)
                    : Common::ZeroMQWrapper::IIPCException(message)
            {}
        };
    }
}


#endif //EVEREST_BASE_ZEROMQWRAPPEREXCEPTION_H
