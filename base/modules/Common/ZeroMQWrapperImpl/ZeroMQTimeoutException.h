//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_ZEROMQTIMEOUTEXCEPTION_H
#define EVEREST_BASE_ZEROMQTIMEOUTEXCEPTION_H


#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        class ZeroMQTimeoutException
                : public Common::ZeroMQWrapper::IIPCTimeoutException
        {
        public:
            explicit ZeroMQTimeoutException(const std::string &message)
                    : Common::ZeroMQWrapper::IIPCTimeoutException(message)
            {}
        };
    }
}


#endif //EVEREST_BASE_ZEROMQTIMEOUTEXCEPTION_H
