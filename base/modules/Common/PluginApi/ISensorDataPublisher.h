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
         * Interface of the DataPublisher.
         * An instance of ISensorDataPublisher can be created via the IPluginResourceManagement.
         *
         * Given an ISensorDataPublisher, data can be published via the ::sendData method.
         *
         * This allows Sensors to create 'lightweight' sensor data to be analysed and processed
         * to detect threats.
         *
         * In order to analyse data, an ISensorDataSubscriber must be instantiated.
         *
         * If no ISensorDataSubscriber is present the data created by the ISensorDataPublisher will be discarded.
         *
         * The publisher never 'blocks' on sending data, but if subscribers fail to process the data and the buffer
         * fills up, old messages will be removed from the buffer to give space to recent messages.
         *
         * The sensorDataCategory will be used against the filters defined by the subscribers.
         */
        class ISensorDataPublisher
        {
        public:
            virtual ~ISensorDataPublisher() = default;

            /**
             * Publishes the data
             * @param sensorDataCategory: The category of the data. Bear in mind that subscribers express their filter as matching prefix of sensorDataCategory.
             * @param sensorData: The content. It is meant to be json (binary or string) containing the full information.
             */
            virtual void sendData(const std::string& sensorDataCategory, const std::string& sensorData) = 0;
        };
    }
}


