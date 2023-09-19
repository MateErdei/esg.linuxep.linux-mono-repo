// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IFileSystemException.h"

namespace Common::FileSystem
{
    class IMoveFileException : public IFileSystemException
    {
    public:
        explicit IMoveFileException(const std::string& what) : IFileSystemException(what) {}
    };
} // namespace Common::FileSystem
