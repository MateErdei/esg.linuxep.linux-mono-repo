/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>
#include <Common/EventTypes/IEventType.h>

namespace Common
{
    namespace PluginApi
    {
        /**
         * Interface of the DataPublisher.
         * An instance of IRawDataPublisher can be created via the IPluginResourceManagement.
         *
         * Given an IRawDataPublisher, data can be published via the ::sendData method.
         *
         * This allows Sensors to create 'lightweight' sensor data to be analysed and processed
         * to detect threats.
         *
         * In order to analyse data, an ISubscriber must be instantiated.
         *
         * If no ISubscriber is present the data created by the IRawDataPublisher will be discarded.
         *
         * The publisher never 'blocks' on sending data, but if subscribers fail to process the data and the buffer
         * fills up, old messages will be removed from the buffer to give space to recent messages.
         *
         * The rawDataCategory will be used against the filters defined by the subscribers.
         */
        class IRawDataPublisher
        {
        public:
            virtual ~IRawDataPublisher() = default;

            /**
             * Publishes the data
             * @param rawDataCategory: The category of the data. Bear in mind that subscribers express their filter as matching prefix of rawDataCategory.
             * @param rawData: The content. It is meant to be json (binary or string) containing the full information.
             */
            virtual void sendData(const std::string& rawDataCategory, const std::string& rawData) = 0;
            virtual void sendEvent(const Common::EventTypes::IEventType&) = 0;
        };
    }
}


