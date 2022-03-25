/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/
#pragma once

#include "Sdds3RepositoryFactory.h"

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/ISdds3Repository.h>

#include <functional>
#include <memory>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConfigurationData;
    }

    using RespositoryCreaterFunc =
    std::function<suldownloaderdata::ISDDS3RepositoryPtr()>;

    /**
     * This class exists to enable an indirection on creating the WarehouseRepository to allow Mocking it out on tests.
     * The WarehouseRepository is the class that encapsulates what SUL lib provides and as such, needs a real remote
     * warehouse to be tested ( or a fake one). In order to enable a component test to SULDownloader that do not depend
     * on SUL lib the WarehouseRepository must be mocked out.
     */
    class Sdds3RepositoryFactory
    {
        /**
         * Design decision: injecting a different IWarehouseRepository (mocking) is to be enabled only in testing.
         * Hence, the mechanism to do so (::replaceCreator) is private, and this class is made friend of the
         * TestWarehouseHelper.
         */
        friend class TestSdds3RepositoryHelper;

    public:
        /**
         * There is only one instance of the factory. Globally accessible.
         * @return Factory instance.
         */
        static Sdds3RepositoryFactory& instance();
        /**
         * In production code, this is just an indirection to WarehouseRepository::FetchConnectedWarehouse
         *
         * But, in testing, it can be replaced by another creation method via the ::replaceCreator.
         *
         * @return
         */
        std::unique_ptr<suldownloaderdata::ISdds3Repository>  createRepository();

    private:
        /**
         * Private as it is is a singleton.
         */
        Sdds3RepositoryFactory();

        /**
         * Ensure that ::fetchConnectedWarehouseRepository is linked to WarehouseRepository::FetchConnectedWarehouse.
         */
        void restoreCreator();
        /**
         * The mechanism to enable injecting another object implementing IWarehouseRepository in SULDownloader.
         * It is to be used in test only via the TestWarehouseHelper.
         * @param creatorMethod
         */
        void replaceCreator(RespositoryCreaterFunc creatorMethod);

        /**
         * The real creator method that is triggered on ::fetchConnectedSdds3Repository
         */
        RespositoryCreaterFunc m_creatorMethod;
    };
} // namespace SulDownloader
