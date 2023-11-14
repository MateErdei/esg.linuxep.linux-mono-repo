// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

#include <iostream>

#include <execinfo.h>
#include <unistd.h>

static void terminate_handler()
{
    void** buffer = new void*[15];
    int count = backtrace(buffer, 15);
    backtrace_symbols_fd(buffer, count, STDERR_FILENO);

    //etc.
    auto ptr = std::current_exception();
    if (ptr)
    {
        try
        {
            std::rethrow_exception(ptr);
        }
        catch (const Common::Exceptions::IException& ex)
        {
            std::cerr << "IException thrown and not caught: " << ex.what_with_location() << '\n';
        }
        catch (const std::system_error& ex)
        {
            std::cerr << "std::system_error thrown and not caught: " << ex.code() << ": " << ex.what() << '\n';
        }
        catch (const std::exception& p)
        {
            std::cerr << "std::exception thrown and not caught: " << p.what() << '\n';
        }
        catch (...)
        {
            std::cerr << "Non-std::exception thrown and not caught\n";
        }
    }
    std::abort();
}
