/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include "OtherSideApi.h"
#include "MessageChannel.h"
namespace CommsComponent
{
    using SimpleReactor = std::function< void( std::shared_ptr<MessageChannel>, OtherSideApi& ) > ; 
    class ReactorAdapter{
        public:
        ReactorAdapter(SimpleReactor, std::string name);
        void onMessageFromOtherSide(std::string); 
        int run(OtherSideApi&);
        private:
        SimpleReactor m_reactor; 
        std::shared_ptr<MessageChannel> m_channel;
        std::string m_name; 
    };
}