// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IFileSystemException.h"

namespace Common::FileSystem
{
    class IPermissionDeniedException : public IFileSystemException
    {
    public:
        using IFileSystemException::IFileSystemException;
    };
} // namespace Common::FileSystem
