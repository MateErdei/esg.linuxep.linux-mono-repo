/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace FileSystem
    {
        class IFileSystemException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;
        };
    } // namespace FileSystem
} // namespace Common
