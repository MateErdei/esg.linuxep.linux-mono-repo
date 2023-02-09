// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "PluginAdapter.h"
#include "PluginUtils.h"

#include "ResponseActions/ResponseActionsImpl/UploadFileAction.h"

namespace ResponsePlugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<TaskQueue> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");

        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.m_taskType)
            {
                case Task::TaskType::STOP:
                    return;
                case Task::TaskType::ACTION:
                    processAction(task.m_content);
                    break;

            }
        }
    }

    void PluginAdapter::processAction(const std::string& action) {
        LOGDEBUG("Process action: " << action);
        ActionType type = PluginUtils::getType(action);
        switch(type)
        {
            case ActionType::UPLOAD_FILE:
                LOGINFO("Running upload");
                doUpload(action);
                break;
            case ActionType::NONE:
                LOGWARN("Unknown action throwing it away");
                break;
        }
    }

    void PluginAdapter::doUpload(const std::string& action)
    {
        ResponseActionsImpl::UploadFileAction uploadFileAction;
        std::string response = uploadFileAction.run(action);
    }

} // namespace ResponsePlugin