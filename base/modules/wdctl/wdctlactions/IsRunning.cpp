/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IsRunning.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>

using namespace wdctl::wdctlactions;

int IsRunning::run()
{
    LOGINFO("Attempting to check plugin is running: " << m_args.m_argument );

    StopAction::IsRunningStatus status = checkIsRunning();
    if( status == StopAction::IsRunningStatus::IsRunning)
    {
        return 0;
    }
    if ( status == StopAction::IsRunningStatus::IsNotRunning)
    {
        return 1;
    }
    return 2;

}
