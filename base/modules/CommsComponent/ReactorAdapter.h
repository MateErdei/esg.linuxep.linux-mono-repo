/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <string>
#include "OtherSideApi.h"
#include "MessageChannel.h"
namespace CommsComponent
{
    using SimpleReactor = std::function<int(std::shared_ptr<MessageChannel>, OtherSideApi &)>;

    /*
     * Implement the interface required for ParentExecutor or ChildExecutor in terms of the SimpleReactor functor above defined.
     */
    class ReactorAdapter
    {
    public:
        ReactorAdapter(SimpleReactor, std::string name);

        // Contract of Executer:
        void onMessageFromOtherSide(std::string);

        void onOtherSideStop();

        int run(OtherSideApi &);

    private:
        SimpleReactor m_reactor;
        std::shared_ptr<MessageChannel> m_channel;
        std::string m_name;
    };
}