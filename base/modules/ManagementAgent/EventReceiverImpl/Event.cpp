// Copyright 2023 Sophos Limited. All rights reserved.

#include "Event.h"

#include <utility>

using namespace ManagementAgent::EventReceiverImpl;

Event::Event(std::string appId, std::string eventXml)
    : appId_(std::move(appId)), eventXml_(std::move(eventXml))
{}
