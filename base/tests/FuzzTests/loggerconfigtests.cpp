/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/SophosLoggerMacros.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>

struct DevNullRedirect
{
    DevNullRedirect() : file("/dev/null")
    {
        strm_buffer = std::cout.rdbuf();
        strm_err_buffer = std::cerr.rdbuf();
        std::cout.rdbuf(file.rdbuf());
        std::cerr.rdbuf(file.rdbuf());
    }
    ~DevNullRedirect()
    {
        std::cout.rdbuf(strm_buffer);
        std::cerr.rdbuf(strm_err_buffer);
    }

    std::ofstream file;
    std::streambuf* strm_buffer;
    std::streambuf* strm_err_buffer;
};

int main()
{
    std::array<uint8_t, 9000> buffer;
    ssize_t read_bytes = ::read(STDIN_FILENO, buffer.data(), buffer.size());
    // failure to read is not to be considered in the parser. Just skip.
    if (read_bytes == -1)
    {
        perror("Failed to read");
        return 1;
    }
    DevNullRedirect devNullRedirect;
    std::string content(buffer.data(), buffer.data() + read_bytes);
    Common::FileSystem::fileSystem()->writeFile("/tmp/base/etc/logger.conf", content);
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, "/tmp");
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup(std::string("anyname"));
    log4cplus::Logger logger = Common::Logging::getInstance("anyname");

    LOG4CPLUS_DEBUG(logger, "test");
    log4cplus::Logger::shutdown();
    return 0;
}
