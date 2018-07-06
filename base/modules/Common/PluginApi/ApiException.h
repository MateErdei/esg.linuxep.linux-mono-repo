/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_APIEXCEPTION_H
#define EVEREST_BASE_APIEXCEPTION_H

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace PluginApi
    {
        /**
         * Exception class to report failures when handling the api requests.
         */
        class ApiException : public Common::Exceptions::IException
        {
        public:
            explicit ApiException(const std::string& what)
                    : Common::Exceptions::IException(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_APIEXCEPTION_H
