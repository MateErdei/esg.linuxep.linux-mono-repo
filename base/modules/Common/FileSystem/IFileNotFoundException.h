// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IFileSystemException.h"

namespace Common::FileSystem
{
    class IFileNotFoundException : public IFileSystemException
    {
    public:
        using IFileSystemException::IFileSystemException;
    };
} // namespace Common::FileSystem
