/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Logging/LoggerConfig.h>

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
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup{Common::Logging::LOGOFFFORTEST()};
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

    try
    {
        (void)SulDownloader::suldownloaderdata::DownloadReport::toReport(content);
    }
    catch (SulDownloader::suldownloaderdata::SulDownloaderException&)
    {
        return 2;
    }
    return 0;
}
