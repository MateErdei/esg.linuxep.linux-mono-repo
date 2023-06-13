// Copyright 2023, Sophos Limited. All rights reserved.

#pragma once

#include "EventWriterWorkerLib/EventWriterWorker.h"
#include "SubscriberLib/Subscriber.h"

namespace Heartbeat
{
    constexpr long MAX_PING_TIMEOUT_SECONDS = std::max(
                                          EventWriterLib::EventWriterWorker::DEFAULT_QUEUE_SLEEP_INTERVAL_MS / 1000,
                                          SubscriberLib::Subscriber::DEFAULT_READ_LOOP_TIMEOUT_MS / 1000) +
                                      3;
}