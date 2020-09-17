///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#include "SophosCppStandard.h"

#include <include/gtest/gtest.h>

extern int versig_main(const std::vector<std::string>& argv);

extern int versig_main(
    int argc,    //[i] Count of arguments
    char* argv[] //[i] Array of argument values
);

#define TESTS "../tests"

namespace
{
    class StringHolder
    {
    public:
        explicit StringHolder(const char* orig) { m_value = strdup(orig); }

        ~StringHolder() { free(m_value); }

        char* get() { return m_value; }

        char* m_value;
    };

    TEST(versig_test, no_args) // NOLINT
    {
        StringHolder name("versig_test");
        ASSERT_NE(name.get(), NULLPTR);
        char* argv[] = { name.get(), NULLPTR };
        int argc = sizeof(argv) / sizeof(char*) - 1;
        int ret = versig_main(argc, argv);
        EXPECT_EQ(ret, 2);
    }

    TEST(versig_test, test_valid) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.valid" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, test_data_files) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.valid",
                                       "-d" TESTS "/data_files/data_good" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, bad_file_signature) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.badsig" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 6);
    }

    TEST(versig_test, bad_certificate_in_manifest) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.badcert1" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 4);
    }

    TEST(versig_test, bad_data_file) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.valid",
                                       "-d" TESTS "/data_files/data_bad" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 5);
    }

    TEST(versig_test, bad_CA_certificate) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.bad",
                                       "-f" TESTS "/data_files/manifest.dat.valid" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 4);
    }

    TEST(versig_test, empty_valid_manifest) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.empty_valid",
                                       "-f" TESTS "/data_files/manifest.dat.empty_valid" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, spaces_in_filename) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.spaces",
                                       "-d" TESTS "/data_files/data_spaces" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, no_sha256) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.nosha256",
                                       "-d" TESTS "/data_files/data_good" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, no_sha256_but_required) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.nosha256",
                                       "-d" TESTS "/data_files/data_good",
                                       "--require-sha256" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 5);
    }

    TEST(versig_test, really_long_comment) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.reallyLongComment",
                                       "-d" TESTS "/data_files/data_good" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, badSHA256) // NOLINT
    {
        std::vector<std::string> argv{ "versig_test",
                                       "-c" TESTS "/cert_files/rootca.crt.valid",
                                       "-f" TESTS "/data_files/manifest.dat.badSHA256",
                                       "-d" TESTS "/data_files/data_good" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 5);
    }


    TEST(versig_test, wrong_root_ca) // NOLINT
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.empty_valid",
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                         "--silent-off"};
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 3);
    }


    TEST(versig_test, linuxStlyeSlashInstallSh) // NOLINT
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.validWithInstallSh",
                                        "--check-install-sh"
        };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    int ret = versig_main(argv);
    EXPECT_EQ(ret, 0);
    }

} // namespace
