/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyAndActionListener.h"

using namespace ManagementAgent::McsRouterPluginCommunicationImpl;

PolicyAndActionListener::PolicyAndActionListener(std::string mcsPath,
        std::shared_ptr<Common::DirectoryWatcher::IDirectoryWatcher> directoryWatcher,
        ManagementAgent::McsRouterPluginCommunicationImpl::ITaskQueueSharedPtr taskQueue,
        ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
    : m_policyListener(mcsPath+"/policy",taskQueue,pluginManager),
      m_actionListener(mcsPath+"/action",taskQueue,pluginManager),
      m_directoryWatcher(directoryWatcher)
{
    directoryWatcher->addListener(m_policyListener);
    directoryWatcher->addListener(m_actionListener);
}

PolicyAndActionListener::~PolicyAndActionListener()
{
    m_directoryWatcher->removeListener(m_policyListener);
    m_directoryWatcher->removeListener(m_actionListener);
}
