///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include "WarehouseRepositoryFactory.h"

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/IWarehouseRepository.h>

#include <functional>
#include <memory>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConfigurationData;
    }

    using WarehouseRespositoryCreaterFunc =
        std::function<suldownloaderdata::IWarehouseRepositoryPtr(const suldownloaderdata::ConfigurationData&)>;

    /**
     * This class exists to enable an indirection on creating the WarehouseRepository to allow Mocking it out on tests.
     * The WarehouseRepository is the class that encapsulates what SUL lib provides and as such, needs a real remote
     * warehouse to be tested ( or a fake one). In order to enable a component test to SULDownloader that do not depend
     * on SUL lib the WarehouseRepository must be mocked out.
     */
    class WarehouseRepositoryFactory
    {
        /**
         * Design decision: injecting a different IWarehouseRepository (mocking) is to be enabled only in testing.
         * Hence, the mechanism to do so (::replaceCreator) is private, and this class is made friend of the
         * TestWarehouseHelper.
         */
        friend class TestWarehouseHelper;

    public:
        /**
         * There is only one instance of the factory. Globally accessible.
         * @return Factory instance.
         */
        static WarehouseRepositoryFactory& instance();
        /**
         * In production code, this is just an indirection to WarehouseRepository::FetchConnectedWarehouse
         *
         * But, in testing, it can be replaced by another creation method via the ::replaceCreator.
         *
         * @return
         */
        suldownloaderdata::IWarehouseRepositoryPtr fetchConnectedWarehouseRepository(
            const suldownloaderdata::ConfigurationData&);

    private:
        /**
         * Private as it is is a singleton.
         */
        WarehouseRepositoryFactory();

        /**
         * Ensure that ::fetchConnectedWarehouseRepository is linked to WarehouseRepository::FetchConnectedWarehouse.
         */
        void restoreCreator();
        /**
         * The mechanism to enable injecting another object implementing IWarehouseRepository in SULDownloader.
         * It is to be used in test only via the TestWarehouseHelper.
         * @param creatorMethod
         */
        void replaceCreator(WarehouseRespositoryCreaterFunc creatorMethod);

        /**
         * The real creator method that is triggered on ::fetchConnectedWarehouseRepository
         */
        WarehouseRespositoryCreaterFunc m_creatorMethod;
    };
} // namespace SulDownloader
