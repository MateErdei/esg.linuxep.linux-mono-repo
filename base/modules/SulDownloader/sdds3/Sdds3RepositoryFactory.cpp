// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Sdds3RepositoryFactory.h"

#include "SDDS3Repository.h"
#include "SusRequester.h"

#include "Common/CurlWrapper/CurlWrapper.h"
#include "HttpRequestsImpl/HttpRequesterImpl.h"

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

    Sdds3RepositoryFactory::Sdds3RepositoryFactory()
    {
        restoreCreator();
    }

    void Sdds3RepositoryFactory::replaceCreator(RespositoryCreaterFunc creatorMethod)
    {
        m_creatorMethod = std::move(creatorMethod);
    }

    void Sdds3RepositoryFactory::restoreCreator()
    {
        m_creatorMethod = []()
        {
            std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curl =
                std::make_shared<Common::CurlWrapper::CurlWrapper>();
            std::shared_ptr<Common::HttpRequests::IHttpRequester> httpClient =
                std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curl);
            auto susRequester = std::make_unique<SDDS3::SusRequester>(httpClient);
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        };
    }

} // namespace SulDownloader