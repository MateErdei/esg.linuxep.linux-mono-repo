/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PubSubPathReplacement.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>

namespace
{
    //this is used to override the normal ApplicationPathManager methods for testing purposes
    class ForTestPathManager: public Common::ApplicationConfigurationImpl::ApplicationPathManager
    {
        std::string getPublisherDataChannelAddress() const override
        {
            return "inproc://datachannelpub.ipc";
        }
        std::string getSubscriberDataChannelAddress() const override
        {
            return "inproc://datachannelsub.ipc";
        }

    };
}

namespace Tests
{


    PubSubPathReplacement::PubSubPathReplacement()
    {
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>( new ForTestPathManager() ));

    }

    PubSubPathReplacement::~PubSubPathReplacement()
    {
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }
}
