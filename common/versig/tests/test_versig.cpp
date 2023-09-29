// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "common/versig/SophosCppStandard.h"

#include <gtest/gtest.h>

extern int versig_main(const std::vector<std::string>& argv);

extern int versig_main(
    int argc,    //[i] Count of arguments
    char* argv[] //[i] Array of argument values
);

#define TESTS "common/versig/tests"

namespace
{
    class StringHolder
    {
    public:
        explicit StringHolder(const char* orig)
        {
            m_value = strdup(orig);
        }

        ~StringHolder()
        {
            free(m_value);
        }

        char* get()
        {
            return m_value;
        }

        char* m_value;
    };

    TEST(versig_test, no_args)
    {
        StringHolder name("versig_test");
        ASSERT_NE(name.get(), NULLPTR);
        char* argv[] = { name.get(), NULLPTR };
        int argc = sizeof(argv) / sizeof(char*) - 1;
        int ret = versig_main(argc, argv);
        EXPECT_EQ(ret, 2);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_test_valid_sha1)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                        "--allow-sha1-signature"};
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, test_valid_sha1_refuses_without_arg)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                        "--silent-off"};
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 6);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_test_data_files)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                        "-d" TESTS "/data_files/data_good",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

    TEST(versig_test, bad_file_signature)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.badsig",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 6);
    }

    TEST(versig_test, bad_certificate_in_manifest)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.badcert1",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 4);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_bad_data_file)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                        "-d" TESTS "/data_files/data_bad",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 5);
    }

    TEST(versig_test, bad_CA_certificate)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.bad",
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 4);
    }

    TEST(versig_test, empty_valid_manifest)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/empty_valid/rootca.crt",
                                        "-f" TESTS "/data_files/manifest.dat.empty_valid"
        };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_spaces_in_filename)
    {
        //Bazel doesn't like files with spaces in them during building
        //Renaming the files during runtine to circumvent this
        std::string source = TESTS;
        source = source + "/data_files/data_spaces/tbp2.txt";
        std::string destination = TESTS;
        destination = destination + "/data_files/data_spaces/tbp 2.txt" ;
        std::rename(source.c_str(), destination.c_str());
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid", // NOLINT(bugprone-suspicious-missing-comma)
                                        "-f" TESTS "/data_files/manifest.dat.spaces",
                                        "-d" TESTS "/data_files/data_spaces",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_no_sha256)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid", // NOLINT
                                        "-f" TESTS "/data_files/manifest.dat.nosha256",
                                        "-d" TESTS "/data_files/data_good",
                                        "--no-require-sha256",
                                        "--allow-sha1-signature"};
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_no_sha256_but_required)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid", // NOLINT
                                        "-f" TESTS "/data_files/manifest.dat.nosha256",
                                        "-d" TESTS "/data_files/data_good",
                                        "--require-sha256",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 5);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_really_long_comment)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid", // NOLINT
                                        "-f" TESTS "/data_files/manifest.dat.reallyLongComment",
                                        "-d" TESTS "/data_files/data_good",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }
    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_badSHA256)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid", // NOLINT
                                        "-f" TESTS "/data_files/manifest.dat.badSHA256",
                                        "-d" TESTS "/data_files/data_good",
                                        "--allow-sha1-signature" };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 5);
    }


    TEST(versig_test, wrong_root_ca)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/empty_valid/rootca.crt", // NOLINT
                                        "-f" TESTS "/data_files/manifest.dat.valid",
                                         "--silent-off",
                                        "--allow-sha1-signature"};
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 3);
    }

    // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_linuxStyleSlashInstallSh)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.validWithInstallSh",
                                        "--check-install-sh",
                                        "--allow-sha1-signature"
        };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

      // TODO: LINUXDAR-8173: Once expired certs and manifests have been updated by this ticket, copy these over and re-enable this test
    TEST(versig_test, DISABLED_windowsStyleSlashInstallSh)
    {
        std::vector<std::string> argv { "versig_test",
                                        "-c" TESTS "/cert_files/rootca.crt.valid",
                                        "-f" TESTS "/data_files/manifest.dat.validWithWindowsInstallSh",
                                        "--check-install-sh",
                                        "--allow-sha1-signature"
        };
        int ret = versig_main(argv);
        EXPECT_EQ(ret, 0);
    }

} // namespace
