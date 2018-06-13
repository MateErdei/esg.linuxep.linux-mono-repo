/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IFILESYSTEMEXCEPTION_H
#define EVEREST_BASE_IFILESYSTEMEXCEPTION_H

#include "Exceptions/IException.h"

namespace Common
{
    namespace FileSystem
    {
        class IFileSystemException : public Common::Exceptions::IException
        {
        public:
            explicit IFileSystemException(const std::string& what)
                    : Common::Exceptions::IException(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_IFILESYSTEMEXCEPTION_H
