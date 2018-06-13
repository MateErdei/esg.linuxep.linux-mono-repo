/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_COMMON_PROCESS_PROCESSEXCEPTION_H
#define EVEREST_BASE_COMMON_PROCESS_PROCESSEXCEPTION_H
#include "Exceptions/IException.h"
namespace Common
{
    namespace Process
    {
        class IProcessException : public Common::Exceptions::IException
        {
        public:
            explicit IProcessException(const std::string& what)
                    : Common::Exceptions::IException(what)
            {}
        };
    }
}


#endif //EVEREST_BASE_COMMON_PROCESS_PROCESSEXCEPTION_H
