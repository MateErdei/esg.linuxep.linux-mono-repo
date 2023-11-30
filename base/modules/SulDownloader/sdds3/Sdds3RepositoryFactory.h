// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Sdds3RepositoryFactory.h"

#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
#include "SulDownloader/suldownloaderdata/ISdds3Repository.h"

#include <functional>
#include <memory>

class TestDownloadReports;
class TestSdds3Repository;

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConfigurationData;
    }

    using RespositoryCreaterFunc = std::function<suldownloaderdata::ISDDS3RepositoryPtr()>;

    /**
     * This class exists to enable an indirection on creating the Sdds3Repository to allow Mocking it out on tests.
     */
    class Sdds3RepositoryFactory
    {
        /**
         * Design decision: injecting a different ISdds3Repository (mocking) is to be enabled only in testing.
         * Hence, the mechanism to do so (::replaceCreator) is private, and this class is made friend of the
         * TestSdds3RepositoryHelper.
         */
        friend class TestSdds3RepositoryHelper;
        friend class ::TestDownloadReports;
        friend class ::TestSdds3Repository;

    public:
        /**
         * There is only one instance of the factory. Globally accessible.
         * @return Factory instance.
         */
        static Sdds3RepositoryFactory& instance();

        std::unique_ptr<suldownloaderdata::ISdds3Repository> createRepository();

    private:
        /**
         * Private as it is is a singleton.
         */
        Sdds3RepositoryFactory();

        void restoreCreator();
        /**
         * The mechanism to enable injecting another object implementing ISdds3Repository in SULDownloader.
         * It is to be used in test only via the TestSdds3RepositoryHelper.
         * @param creatorMethod
         */
        void replaceCreator(RespositoryCreaterFunc creatorMethod);

        /**
         * The real creator method that is triggered on ::fetchConnectedSdds3Repository
         */
        RespositoryCreaterFunc m_creatorMethod;
    };
} // namespace SulDownloader
