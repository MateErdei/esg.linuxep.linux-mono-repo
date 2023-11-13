// Copyright 2023 Sophos All rights reserved.
#include "MountFactory.h"

#include "Mounts.h"

#include <utility>

using namespace mount_monitor::mountinfoimpl;
using namespace mount_monitor::mountinfo;

MountFactory::MountFactory(ISystemPathsFactorySharedPtr pathsFactory)
    : pathsFactory_(std::move(pathsFactory))
{
}

IMountInfoSharedPtr MountFactory::newMountInfo()
{
    return std::make_shared<Mounts>(pathsFactory_->createSystemPaths());
}
