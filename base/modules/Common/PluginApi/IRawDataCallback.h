/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace Common
{
    namespace PluginApi
    {
        /**
         * Class that must be implemented when interested in subscribing to RawData.
         * @see IRawDataPublisher and ISubscriber.
         *
         * When data arrives in the ISubscriber that data will be forwarded to the
         * ::receiveData method of this callback.
         */
        class IRawDataCallback
        {
        public:
            virtual ~IRawDataCallback() = default;

            /**
             * It is via this method that the ISubscriber will pass data new subscription data.
             * @param key This is the rawDataCategory as emitted by the publisher ( @see IRawDataPublisher::sendData)
             * @param data The data content. It is meant to be a json (binary or string) containing relevant information.
             */
            virtual void receiveData(const std::string& key, const std::string& data) = 0;
        };
    }
}
