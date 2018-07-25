/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef COMMON_PROCESS_IPROCESSEXCEPTION_H
#define COMMON_PROCESS_IPROCESSEXCEPTION_H
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


#endif //COMMON_PROCESS_IPROCESSEXCEPTION_H
