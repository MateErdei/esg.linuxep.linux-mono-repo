// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common::DirectoryWatcher
{
    class IDirectoryWatcherException : public Common::Exceptions::IException
    {
    public:
        explicit IDirectoryWatcherException(const std::string& what) : Common::Exceptions::IException(what) {}
    };
} // namespace Common::DirectoryWatcher
