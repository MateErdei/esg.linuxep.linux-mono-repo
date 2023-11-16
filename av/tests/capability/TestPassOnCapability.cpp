// Copyright 2021-2023 Sophos Limited. All rights reserved.

// We need to get setresuid and setresgid
// If we ever need to support non-Linux then we'll have to re-evaluate
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "common/SaferStrerror.h"

#include "products/capability/PassOnCapability.h"

#include "datatypes/Print.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include <sys/ptrace.h>
#include <unistd.h>

TEST(TestPassOnCapability, pass_on_capability_root_only)
{
    if (::geteuid() != 0)
    {
        GTEST_SKIP() << "Test requires root";
    }

    int ret = pass_on_capability(CAP_SYS_CHROOT);
    EXPECT_EQ(ret, 0);
}

#define DROP_ROOT

namespace
{
    std::string perms_to_string(std::filesystem::perms p)
    {
        using std::filesystem::perms;
        std::ostringstream ost;
        auto show = [&](char op, perms perm)
        {
            ost << (perms::none == (perm & p) ? '-' : op);
        };
        show('r', perms::owner_read);
        show('w', perms::owner_write);
        show('x', perms::owner_exec);
        show('r', perms::group_read);
        show('w', perms::group_write);
        show('x', perms::group_exec);
        show('r', perms::others_read);
        show('w', perms::others_write);
        show('x', perms::others_exec);
        return ost.str();
    }

    void ensureWritable(const std::filesystem::path& path)
    {
        using namespace std::filesystem;
        PRINT("Ensure " << path.c_str() << " is writable");
        std::error_code ec;
        permissions(path,
                    perms::others_read | perms::others_write | perms::group_read | perms::group_write,
                    perm_options::add,
                    ec
                    );
        if (ec)
        {
            PRINT("Failed to chmod "<< path << ": " << ec.category().name() << " " << ec.value());
        }
        auto permission = perms::all;
        auto target = path;
        while (true)
        {
            auto parent = target.parent_path();
            if (parent == target)
            {
                break;
            }
            permissions(parent,
                        permission,
                        perm_options::add,
                        ec
            );
            if (ec)
            {
                PRINT("Failed to chmod " << parent << ": " << ec.category().name() << " " << ec.value());
            }
            else
            {
                PRINT("chmod " << parent << " adding " << perms_to_string(permission));
            }
            permission = perms::others_exec | perms::group_exec;
            target = std::move(parent);
        }
    }

    void printPermissions(std::filesystem::path path)
    {
        if (path.empty())
        {
            return;
        }
        namespace fs = std::filesystem;
        while (true)
        {
            auto status = fs::status(path);
            if (status.type() == fs::file_type::not_found)
            {
                PRINT(path << " does not exist");
            }
            else
            {
                auto perm = status.permissions();
                PRINT(path << " = " << perms_to_string(perm));
            }
            auto parent = path.parent_path();
            if (parent == path || parent.empty())
            {
                break;
            }
            path = std::move(parent);
        }
    }

    class RestoreRoot
    {
    public:
        bool restoreRoot_{false};
        std::string xmlOutputFile_;
        RestoreRoot()
        {
            if (::geteuid() == 0)
            {
#ifdef DROP_ROOT
                PRINT("Drop root fully");
                char* XML_OUTPUT_FILE = getenv("XML_OUTPUT_FILE");
                if (XML_OUTPUT_FILE)
                {
                    ensureWritable(XML_OUTPUT_FILE);
                    xmlOutputFile_ = XML_OUTPUT_FILE;
                }
                else
                {
                    PRINT("XML_OUTPUT_FILE not defined!");

                    for (char** env = environ; *env; ++env)
                    {
                        PRINT(*env);
                    }
                    PRINT("PWD:" << get_current_dir_name());

                }

                int ret = setresgid(1, 1, 1);
                EXPECT_EQ(ret, 0) << "setresgid failed: ret=" << ret << " errno=" << errno << " strerror="  << common::safer_strerror(errno);
                ret = setresuid(1, 1, 1);
                EXPECT_EQ(ret, 0) << "setresuid failed: ret=" << ret << " errno=" << errno << " strerror="  << common::safer_strerror(errno);
                if (ret == -1)
                {
                    {
                        PRINT("/proc/self/status:");
                        std::ifstream fstream{"/proc/self/status"};
                        std::string line;
                        while (std::getline(fstream, line))
                        {
                            PRINT(line);
                        }
                    }
                    {
                        PRINT("/proc/self/uid_map:");
                        std::ifstream fstream{"/proc/self/uid_map"};
                        std::string line;
                        while (std::getline(fstream, line))
                        {
                            PRINT(line);
                        }
                    }

                }
                EXPECT_EQ(::geteuid(), 1);
                EXPECT_EQ(::getuid(), 1);
#else
                PRINT("Restorably drop root");
                int ret = setresgid(1, 1, 0);
                EXPECT_EQ(ret, 0);
                ret = setresuid(1, 1, 0);
                EXPECT_EQ(ret, 0);
                restoreRoot_ = true;
#endif
            }
            printPermissions(xmlOutputFile_);
        }

        ~RestoreRoot()
        {
            if (restoreRoot_)
            {
                PRINT("Restore root");
                int ret = setresuid(0, 0, 0);
                EXPECT_EQ(ret, 0) << "setresgid failed: ret=" << ret << " errno=" << errno << " strerror="  << common::safer_strerror(errno);
                ret = setresgid(0, 0, 0);
                EXPECT_EQ(ret, 0) << "setresuid failed: ret=" << ret << " errno=" << errno << " strerror="  << common::safer_strerror(errno);
            }

            printPermissions(xmlOutputFile_);
        }
    };
}

TEST(TestPassOnCapability, pass_on_capability_no_root)
{
    RestoreRoot restoreRoot;

    ASSERT_NE(::geteuid(), 0);
    ASSERT_NE(::getuid(), 0);

    int ret = pass_on_capability(CAP_SYS_CHROOT);
    EXPECT_NE(ret, 0);
    // non-root process returns 62 - cap_set_proc fails
    // ex-root process returns 64 - cap_set_ambient fails - cap_set_proc works
}

TEST(TestPassOnCapability, no_new_privs)
{
    set_no_new_privs();
}