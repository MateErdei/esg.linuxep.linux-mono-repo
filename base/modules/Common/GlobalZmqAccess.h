
#pragma once

#include <set>

//static std::vector<std::weak_ptr<int>> GL_zmqs;
static std::set<void*> GL_zmqSockets;
static std::set<void*> GL_zmqContexts;
