// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "ConnectionSetup.h"
#include "IRepository.h"

#include "SulDownloader/suldownloaderdata/ConnectionSetup.h"

#include <bits/unique_ptr.h>

#include <set>
#include <string>
#include <vector>

namespace SulDownloader::suldownloaderdata
{
    /**
     * Interface for SDDS3Repository to enable tests.
     * For documentation, see SDDS3Repository.h
     */
    class ISdds3Repository : public IRepository
    {
    public:
        virtual ~ISdds3Repository() = default;

        virtual bool synchronize(
            const Common::Policy::UpdateSettings& updateSettings,
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            const bool ignoreFailedSupplementRefresh) = 0;

        virtual void distribute() = 0;
        //
        //            /**
        //             * Attempt to connect to a provided connection setup information.
        //             *
        //             *
        //             * @param connectionSetup
        //             * @param supplementOnly  Only download supplements
        //             * @param configurationData
        //             * @return
        //             */
        virtual bool tryConnect(
            const suldownloaderdata::ConnectionSetup& connectionSetup,
            bool supplementOnly,
            const Common::Policy::UpdateSettings& updateSettings) = 0;
        virtual void setWillInstall(const bool willInstall) = 0;
    };

    using ISDDS3RepositoryPtr = std::unique_ptr<ISdds3Repository>;

} // namespace SulDownloader::suldownloaderdata
