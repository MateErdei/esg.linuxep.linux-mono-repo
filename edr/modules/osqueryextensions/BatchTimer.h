/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include "LoggerPlugin/BatchTimerInterface.h"

#include <condition_variable>
#include <future>
#include <mutex>
#include <string>

class BatchTimer : public BatchTimerInterface
{
public:
    BatchTimer();
    ~BatchTimer();

    void Start() override;
    void Cancel() override;

    void Configure(std::function<void()> callback, std::chrono::milliseconds maxBatchTime) override;

    std::string getCallbackError() const { return m_callbackError; }

private:
    void run();
    void reset();

    std::function<void()> m_callback;
    std::chrono::milliseconds m_maxBatchTime;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    std::future<void> m_future;
    std::string m_callbackError;
    bool m_cancelled;
};
