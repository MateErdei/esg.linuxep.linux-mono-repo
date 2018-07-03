//
// Created by pair on 02/07/18.
//

#ifndef EVEREST_BASE_APIEXCEPTION_H
#define EVEREST_BASE_APIEXCEPTION_H

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace PluginApi
    {
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
