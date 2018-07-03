//
// Created by pair on 29/06/18.
//

#ifndef EVEREST_BASE_ISENSORDATASUBSCRIBER_H
#define EVEREST_BASE_ISENSORDATASUBSCRIBER_H
#include <memory>
#include "ISensorDataCallback.h"
namespace Common
{
    namespace PluginApi
    {
        class ISensorDataSubscriber
        {
        public:
            static std::unique_ptr<ISensorDataSubscriber> newSensorDataSubscriber(const std::string & sensorDataCategorySubscription,
                                                                                  std::shared_ptr<ISensorDataCallback> sensorDataCallback);

        };
    }
}

#endif //EVEREST_BASE_ISENSORDATASUBSCRIBER_H
