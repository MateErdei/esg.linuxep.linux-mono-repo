/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ZMQAction.h"

#include <Common/ZeroMQWrapper/IContext.h>

wdctl::wdctlactions::ZMQAction::ZMQAction(const wdctl::wdctlarguments::Arguments& args)
        : Action(args),m_context(Common::ZeroMQWrapper::createContext())
{
}
