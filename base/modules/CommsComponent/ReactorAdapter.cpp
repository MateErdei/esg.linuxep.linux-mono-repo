/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ReactorAdapter.h"
#include "Logger.h"
namespace CommsComponent
{

    ReactorAdapter::ReactorAdapter(SimpleReactor reactor, std::string name) :
            m_reactor(std::move(reactor)),
            m_channel{std::make_shared<MessageChannel>()},
            m_name(std::move(name))
    {
    }

    void ReactorAdapter::onMessageFromOtherSide(std::string string)
    {
        m_channel->push(std::move(string));
    }

    void ReactorAdapter::onOtherSideStop()
    {
        m_channel->pushStop();
    }

    int ReactorAdapter::run(OtherSideApi &othersideApi)
    {
        othersideApi.setNotifyOnClosure([this]() { m_channel->pushStop(); });
        try
        {
            int exitCode = m_reactor(m_channel, othersideApi);
            othersideApi.setNotifyOnClosure([]() {});
            return exitCode;
        }
        catch (const ChannelClosedException &channelClosedException)
        { //explicitly capture the ChannelClosedException as a normal case scenario.
            LOGDEBUG(channelClosedException.what());
            return 0;
        }
        catch (std::exception &ex)
        {
            othersideApi.setNotifyOnClosure([]() {});
            LOGERROR(m_name << " process reported error: " << ex.what());
            return EXIT_FAILURE;
        }

    }
} 