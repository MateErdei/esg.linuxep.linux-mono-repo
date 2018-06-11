//
// Created by pair on 08/06/18.
//

#ifndef EVEREST_BASE_IIPCEXCEPTION_H
#define EVEREST_BASE_IIPCEXCEPTION_H

#include <Common/Exceptions/IException.h>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IIPCException
            : public Common::Exceptions::IException
        {
        public:
            explicit IIPCException(const std::string& what)
                    : Common::Exceptions::IException(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_IIPCEXCEPTION_H
