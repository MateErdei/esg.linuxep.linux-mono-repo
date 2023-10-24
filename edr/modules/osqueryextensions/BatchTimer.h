// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/LoggerPlugin/BatchTimerInterface.h"
#else
#include "LoggerPlugin/BatchTimerInterface.h"
#endif

#include <condition_variable>
#include <future>
#include <mutex>
#include <string>

class BatchTimer : public BatchTimerInterface
{
public:
    BatchTimer();
    ~BatchTimer() override;

    void Start() override;
    void Cancel() final; // called from destructor

    void Configure(std::function<void()> callback, std::chrono::milliseconds maxBatchTime) override;

    [[nodiscard]] std::string getCallbackError() const { return m_callbackError; }

private:
    void run();
    void reset();

    std::function<void()> m_callback;
    std::chrono::milliseconds m_maxBatchTime{0};
    std::condition_variable m_cv;
    std::mutex m_mutex;
    std::future<void> m_future;
    std::string m_callbackError;
    bool m_cancelled{false};
};
