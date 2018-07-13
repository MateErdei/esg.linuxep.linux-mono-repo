/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_MESSAGEEXCEPTION_H
#define EVEREST_BASE_MESSAGEEXCEPTION_H

#include "Exceptions/IException.h"

namespace Common
{
    namespace UtilityImpl
    {
        class MessageException : public Common::Exceptions::IException
        {
        public:
            explicit MessageException(const std::string &what)
                    : Common::Exceptions::IException(what)
            {}
        };

    }
}

#endif //EVEREST_BASE_MESSAGEEXCEPTION_H
