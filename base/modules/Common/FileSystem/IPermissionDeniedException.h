// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IFileSystemException.h"

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
