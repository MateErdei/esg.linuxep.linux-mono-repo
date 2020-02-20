/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseFileWalkCallbacks.h"


using namespace avscanner::avscannerimpl;

BaseFileWalkCallbacks::BaseFileWalkCallbacks(ScanClient scanner)
    : m_scanner(std::move(scanner))
{
}
