/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IRepository.h"

#include <bits/unique_ptr.h>

#include <set>
#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        /**
         * Interface for SDDS3Repository to enable tests.
         * For documentation, see SDDS3Repository.h
         */
        class ISdds3Repository : public IRepository
        {
        public:

            virtual ~ISdds3Repository() = default;

            virtual void synchronize(const ConfigurationData& configurationData) = 0;
//                                     std::set<std::string>& suites,
//                                     std::set<std::string>& releaseGroups) = 0;

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
                const suldownloaderdata::ConfigurationData& configurationData) = 0;
        };

        using ISDDS3RepositoryPtr = std::unique_ptr<ISdds3Repository>;

    } // namespace suldownloaderdata
} // namespace SulDownloader
