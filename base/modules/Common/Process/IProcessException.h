//
// Created by pair on 12/06/18.
//

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
