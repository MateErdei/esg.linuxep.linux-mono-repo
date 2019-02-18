/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common
{
    namespace FileSystem
    {
        class IPermissionDeniedException : public IFileSystemException
        {
        public:
            explicit IPermissionDeniedException(const std::string& what) : IFileSystemException(what) {}
        };
    } // namespace FileSystem
} // namespace Common
