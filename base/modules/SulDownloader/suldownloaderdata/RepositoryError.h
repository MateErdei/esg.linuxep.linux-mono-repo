// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/DownloadReport/RepositoryStatus.h"

#include <string>

namespace SulDownloader::suldownloaderdata
{
    /**
     * Struct to keep track of report of failures both of the SUL library and the SULDownloader.
     *
     */
    struct RepositoryError
    {
        std::string Description; /// Description of failure provided by SULDownloader
        std::string LibError;    /// Description of failure provided by Lib library.
        Common::DownloadReport::RepositoryStatus status;

        void reset()
        {
            Description = "";
            LibError = "";
            status = Common::DownloadReport::RepositoryStatus::SUCCESS;
        }
    };
} // namespace SulDownloader::suldownloaderdata
