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

void susiLogCallback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    if (!m.empty())
    {
        LOGERROR(level << ": " << m);
    }
}

static const SusiLogCallback GL_log_callback{
    .version = SUSI_LOG_CALLBACK_VERSION,
    .token = nullptr,
    .log = susiLogCallback,
    .minLogLevel = SUSI_LOG_LEVEL_DETAIL
};

static void throwIfNotOk(SusiResult res, const std::string& message)
{
    if (res != SUSI_S_OK)
    {
        throw std::runtime_error(message);
    }
}

SusiGlobalHandlerSharePtr SusiWrapperFactory::getGlobalHandler(const std::string& runtimeConfig)
{
    if (!m_globalHandler)
    {
        auto res = SUSI_SetLogCallback(&GL_log_callback);
        throwIfNotOk(res, "Failed to set log callback");

        m_globalHandler = std::make_shared<SusiGlobalHandler>(runtimeConfig);
    }
    return m_globalHandler;
}

SusiWrapperFactory::SusiWrapperFactory()
{

}
