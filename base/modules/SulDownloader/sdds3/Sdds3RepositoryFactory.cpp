// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Sdds3RepositoryFactory.h"

#include "SDDS3Repository.h"
#include "SignatureVerifierWrapper.h"
#include "SusRequester.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"

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
            auto signatureVerifierWrapper = std::make_unique<SignatureVerifierWrapper>(
                Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath());
            auto susRequester = std::make_unique<SDDS3::SusRequester>(httpClient, std::move(signatureVerifierWrapper));
            return std::make_unique<SDDS3Repository>(std::move(susRequester));
        };
    }

} // namespace SulDownloader