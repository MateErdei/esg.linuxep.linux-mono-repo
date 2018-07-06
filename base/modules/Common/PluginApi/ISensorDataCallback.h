/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ISENSORDATACALLBACK_H
#define EVEREST_BASE_ISENSORDATACALLBACK_H
#include <string>
namespace Common
{
    namespace PluginApi
    {
        /**
         * Class that must be implemented when interested in subscribing to SensorData.
         * @see ISensorDataPublisher and ISensorDataSubscriber.
         *
         * When data arrives in the ISensorDataSubscriber that data will be forwarded to the
         * ::receiveData method of this callback.
         */
        class ISensorDataCallback
        {
        public:
            virtual ~ISensorDataCallback() = default;
            /**
             * It is via this method that the ISensorDataSubscriber will pass data new subscription data.
             * @param key This is the sensorDataCategory as emitted by the publisher ( @see ISensorDataPublisher::sendData)
             * @param data The data content. It is meant to be a json (binary or string) containing relevant information.
             */
            virtual void receiveData(const std::string &key, const std::string &data) = 0;
        };
    }
}
#endif //EVEREST_BASE_ISENSORDATACALLBACK_H
