/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IRepository.h"

#include <bits/unique_ptr.h>

#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        /**
         * Interface for WarehouseRepository to enable tests.
         * For documentation, see WarehouseRepository.h
         */
        class IWarehouseRepository : public IRepository
        {
        public:
            using SulLogsVector = std::vector<std::string>;

            virtual ~IWarehouseRepository() = default;

            virtual void synchronize(ProductSelection&) = 0;

            virtual void distribute() = 0;

            /**
             * Attempt to connect to a provided connection setup information.
             *
             *
             * @param connectionSetup
             * @param supplementOnly  Only download supplements
             * @param configurationData
             * @return
             */
            virtual bool tryConnect(
                const suldownloaderdata::ConnectionSetup& connectionSetup,
                bool supplementOnly,
                const suldownloaderdata::ConfigurationData& configurationData) = 0;

            /**
             * Get the logs for this SUL connection
             * @return
             */
            virtual SulLogsVector getLogs() const = 0;

            virtual void dumpLogs() const = 0;

            virtual void reset() = 0;

        };

        using IWarehouseRepositoryPtr = std::unique_ptr<IWarehouseRepository>;

    } // namespace suldownloaderdata
} // namespace SulDownloader
