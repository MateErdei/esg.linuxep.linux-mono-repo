/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapperFactory.h"

#include "SusiWrapper.h"
#include "Logger.h"

using namespace threat_scanner;

std::shared_ptr<ISusiWrapper> SusiWrapperFactory::createSusiWrapper(const std::string& runtimeConfig, const std::string& scannerConfig)
{
    return std::make_shared<SusiWrapper>(getGlobalHandler(runtimeConfig), scannerConfig);
}

SusiGlobalHandlerSharePtr SusiWrapperFactory::getGlobalHandler(const std::string& runtimeConfig)
{
    if (!m_globalHandler)
    {
        m_globalHandler = std::make_shared<SusiGlobalHandler>(runtimeConfig);
    }
    return m_globalHandler;
}

SusiWrapperFactory::SusiWrapperFactory()
{
}
