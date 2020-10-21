/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "datatypes/AutoFd.h"

#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace
{
    class TestFile
    {
    public:
        explicit TestFile(const char* name) : m_name(name)
        {
            ::unlink(m_name.c_str());
            datatypes::AutoFd fd(
                ::open(m_name.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR)); // NOLINT(hicpp-signed-bitwise)
        }

        ~TestFile()
        {
            ::unlink(m_name.c_str());
        }

        [[nodiscard]] int open(int oflag = O_RDONLY) const
        {
            datatypes::AutoFd fd(::open(m_name.c_str(), oflag));
            return fd.release();
        }

    protected:
        std::string m_name;
    };

}