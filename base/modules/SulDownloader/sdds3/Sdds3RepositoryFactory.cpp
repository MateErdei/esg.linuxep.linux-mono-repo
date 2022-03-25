/***********************************************************************************************

Copyright 2022-2022 Sophos Limited. All rights reserved.

***********************************************************************************************/
#include "Sdds3RepositoryFactory.h"

#include "SDDS3Repository.h"

using namespace SulDownloader::suldownloaderdata;

namespace SulDownloader
{
    Sdds3RepositoryFactory& Sdds3RepositoryFactory::instance()
    {
        static Sdds3RepositoryFactory factory;
        return factory;
    }

    std::unique_ptr<ISdds3Repository> Sdds3RepositoryFactory::createRepository()
    {
        return m_creatorMethod();
    }

    Sdds3RepositoryFactory::Sdds3RepositoryFactory() { restoreCreator(); }

    void Sdds3RepositoryFactory::replaceCreator(RespositoryCreaterFunc creatorMethod)
    {
        m_creatorMethod = std::move(creatorMethod);
    }

    void Sdds3RepositoryFactory::restoreCreator()
    {
        m_creatorMethod = []() {
          return std::make_unique<SDDS3Repository>();
        };
    }

} // namespace SulDownloader