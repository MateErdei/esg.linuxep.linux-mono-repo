// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <gmock/gmock.h>
#include <nlohmann/json.hpp>

MATCHER_P2(HasSingleComponentWithWarehouseAndInstalledVersion, warehouseVersion, installedVersion, "")
{
    const auto json = nlohmann::json::parse(arg);
    const auto component = json.at("warehouseComponents").at(0);
    return json.at("warehouseComponents").size() == 1 && component.at("warehouseVersion") == warehouseVersion &&
           component.at("installedVersion") == installedVersion;
}