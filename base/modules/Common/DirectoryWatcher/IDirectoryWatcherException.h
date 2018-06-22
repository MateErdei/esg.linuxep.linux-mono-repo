/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IDIRECTORYWATCHEREXCEPTION_H
#define EVEREST_BASE_IDIRECTORYWATCHEREXCEPTION_H

#include "Exceptions/IException.h"

namespace Common
{
    namespace DirectoryWatcher
    {
        class IDirectoryWatcherException : public Common::Exceptions::IException
        {
        public:
            explicit IDirectoryWatcherException(const std::string& what) :
                   Common::Exceptions::IException(what)
            {}
        };
    }
}

#endif //EVEREST_BASE_IDIRECTORYWATCHEREXCEPTION_H
