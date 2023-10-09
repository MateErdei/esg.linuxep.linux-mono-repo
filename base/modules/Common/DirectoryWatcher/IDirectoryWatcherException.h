/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "modules/Common/Exceptions/IException.h"

namespace Common
{
    namespace DirectoryWatcher
    {
        class IDirectoryWatcherException : public Common::Exceptions::IException
        {
        public:
            explicit IDirectoryWatcherException(const std::string& what) : Common::Exceptions::IException(what) {}
        };
    } // namespace DirectoryWatcher
} // namespace Common
