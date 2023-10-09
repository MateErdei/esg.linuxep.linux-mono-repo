// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include <gtest/gtest.h>
#include <modules/Telemetry/TelemetryImpl/ISystemTelemetryCollector.h>
#include <modules/Telemetry/TelemetryImpl/ITelemetryProvider.h>
#include <modules/Telemetry/TelemetryImpl/TelemetryProcessor.h>

class DerivedTelemetryProcessor : public Telemetry::TelemetryProcessor
{
public:
    DerivedTelemetryProcessor(
        std::shared_ptr<const Common::TelemetryConfigImpl::Config> config,
        std::shared_ptr<Common::HttpRequests::IHttpRequester> httpRequester,
        std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders) :
        TelemetryProcessor(std::move(config), std::move(httpRequester), std::move(telemetryProviders))
    {
    }

    void gatherTelemetry() override { Telemetry::TelemetryProcessor::gatherTelemetry(); }
    std::string getSerialisedTelemetry() override { return Telemetry::TelemetryProcessor::getSerialisedTelemetry(); }
    void sendTelemetry(const std::string& telemetryJson) override
    {
        Telemetry::TelemetryProcessor::sendTelemetry(telemetryJson);
    }
    void saveTelemetry(const std::string& telemetryJson) const override
    {
        Telemetry::TelemetryProcessor::saveTelemetry(telemetryJson);
    }
    void addTelemetry(const std::string& sourceName, const std::string& json) override
    {
        Telemetry::TelemetryProcessor::addTelemetry(sourceName, json);
    }
};
