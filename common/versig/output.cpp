// Copyright 2023 Sophos All rights reserved.
#include "output.h"

#include <iostream>

static bool g_bSilent = true; // Silent by default


void manifest::setSilent(bool s)
{
    g_bSilent = s;
}

/**
 *
// Output message if not in silent mode.
// NB: Output in English. Use for debug only.
 * @param Msg
 */
void manifest::Output(const std::string& Msg)
{
    if (!g_bSilent)
    {
        std::cout << Msg << '\n';
    }
}
