/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


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


