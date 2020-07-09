/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ReactorAdapter.h"
#include "Logger.h"
namespace CommsComponent {

    ReactorAdapter::ReactorAdapter(SimpleReactor reactor, std::string name):
        m_reactor(reactor), 
        m_channel{std::make_shared<MessageChannel>()},
        m_name(std::move(name))
    {        

    }
    void ReactorAdapter::onMessageFromOtherSide(std::string string)
    {
        m_channel->push(std::move(string)); 
    } 

    int ReactorAdapter::run(OtherSideApi& othersideApi){
        othersideApi.setNofifyOnClosure([this](){ m_channel->pushStop();} );
        try{
            m_reactor(m_channel, othersideApi); 
            othersideApi.setNofifyOnClosure([](){});
            return 0; 
        }catch(std::exception & ex)
        {
            othersideApi.setNofifyOnClosure([](){});
            LOGERROR( m_name << " process reported error: " << ex.what()); 
            return 1; 
        }
        
    }
} 