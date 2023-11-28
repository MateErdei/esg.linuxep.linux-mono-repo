// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "ExampleManifests.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Installer/ManifestDiff/Manifest.h"
#include "Installer/ManifestDiff/ManifestDiff.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    Installer::ManifestDiff::Manifest manifestFromString(const std::string& s)
    {
        std::stringstream ost(s);
        return Installer::ManifestDiff::Manifest(ost);
    }

    class TestManifestDiff : public ::testing::Test
    {
    };

    // cppcheck-suppress syntaxError
    TEST_F(TestManifestDiff, calculateAddedWorksCorrectly)
    {
        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

        MockFileSystem mockFileSystem;
        Installer::ManifestDiff::ManifestDiff manifestDiff{ mockFileSystem };

        Installer::ManifestDiff::PathSet added(manifestDiff.calculateAdded(old_manifest, new_manifest));
        ASSERT_EQ(added.size(), 1);

        EXPECT_EQ(*(added.begin()), "files/base/bin/manifestdiff");
    }

    TEST_F(TestManifestDiff, writeAdded)
    {
        StrictMock<MockFileSystem> mockFileSystem;

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

        std::string dest = "test";

        EXPECT_CALL(mockFileSystem, makedirs(_));
        EXPECT_CALL(mockFileSystem, writeFile(dest, "files/base/bin/manifestdiff\n"));

        Installer::ManifestDiff::ManifestDiff manifestDiff{ mockFileSystem };

        EXPECT_NO_THROW(manifestDiff.writeAdded(dest, old_manifest, new_manifest));
    }

    TEST_F(TestManifestDiff, writeRemoved)
    {
        StrictMock<MockFileSystem> mockFileSystem;

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(two_entries));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry));

        std::string dest = "test";

        EXPECT_CALL(mockFileSystem, makedirs(_));
        EXPECT_CALL(mockFileSystem, writeFile(dest, "files/base/bin/manifestdiff\n"));

        Installer::ManifestDiff::ManifestDiff manifestDiff{ mockFileSystem };

        EXPECT_NO_THROW(manifestDiff.writeRemoved(dest, old_manifest, new_manifest));
    }

    TEST_F(TestManifestDiff, writeChanged)
    {
        StrictMock<MockFileSystem> mockFileSystem;

        Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
        Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry_changed));

        std::string dest = "test";

        EXPECT_CALL(mockFileSystem, makedirs(_));
        EXPECT_CALL(mockFileSystem, writeFile(dest, "files/base/bin/SulDownloader\n"));

        Installer::ManifestDiff::ManifestDiff manifestDiff{ mockFileSystem };

        EXPECT_NO_THROW(manifestDiff.writeChanged(dest, old_manifest, new_manifest));
    }

    TEST_F(TestManifestDiff, OldManifestMissingDoesNotCauseFailure)
    {
        MockFileSystem mockFileSystem;

        const std::vector<std::string> args{
            "manifestdiff",          "--old=old.dat",     "--new=new.dat",
            "--changed=changed.dat", "--added=added.dat", "--removed=removed.dat",
        };

        auto newData =
            std::make_unique<std::stringstream>(R"("./VERSION.ini" 142 9ae706fcba9da1cce5eb43666c5034189316cda6
#sha256 5115c6ec95275803868f24b1d3e4375df2e17f5f978b681f06519073fe4d68c1
#sig sha384 mmegu3RG123zn5ozXfgHMjkIDUpAFAHM5X3lzy+6msSqbxBEt3/juutrWYR2iy+hfwzjCoq6V+dyCn7Zm93SFeVRmj+vXmB243Z9OKAP5AeOVNa83sw6Pw0HNwh0eE+tggKGQHR24QZOJiixk1cD6IQKi9CAEj/f0+wgAorSKfqcOLuLlcyiXx4POGnOxErnShKfTpV471eEnq+aXK64YdIt6L5uOLIRBciQATN3lE+A1XdZnP2okHUfoXtGx6Av3kODprqQXK1/gsFocEe1/rumC7L4Kyg1GBN//GIV0M01KerUYRIQlp92sc88mKyD9B/dw6rxHg+xN/YNeV321Q==
#... cert MIIDojCCAoqgAwIBAgIBNzANBgkqhkiG9w0BAQwFADCBhzEfMB0GA1UEAwwWU29waG9zIEJ1aWxkIEF1dGhvcml0eTEiMCAGCSqGSIb3DQEJARYTc29waG9zY2FAc29waG9zLmNvbTEUMBIGA1UECAwLT3hmb3Jkc2hpcmUxCzAJBgNVBAYTAlVLMQ8wDQYDVQQKDAZTb3Bob3MxDDAKBgNVBAsMA0VTRzAeFw0yMTAxMDEwMDAwMDFaFw0zNTA5MTMwMDAwMDBaMIGRMRswGQYDVQQDDBJEYWlseSBCdWlsZCBzaGEzODQxJDAiBgkqhkiG9w0BCQEWFURhaWx5QnVpbGRAc29waG9zLmNvbTEUMBIGA1UECAwLT3hmb3Jkc2hpcmUxCzAJBgNVBAYTAlVLMRMwEQYDVQQKDApTb3Bob3MgUGxjMRQwEgYDVQQLDAtEZXZlbG9wbWVudDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALkff8z24cIf2qhkfRc7KDBTXh5abwVLjCGzA3okmpQJy+3ena/Rrt+wChLAQciv+tgcry0tymqxhgpMwYezNvigZWNsIurVwBI1aXDB2rYvi10npZLsLHSbUErWtc1bv2xtcHSbhlFAcDQ2b7P7JCYCVKfoJ72qj3wNakpFxVeCYr58WaSkwe05JqXUBV6C8iwbS3dHca6vebdXB1cRl3OCT96I7q7hXUcpPCl8IkMdk4cocVKRW4JV1C2OncBNRTbQSDp/0xhQOozUWaxafN6cxyEkeQHQ972OJucOMwTrhFo8CDtLrzCCePbsEJy7KWsaHRVdP2x2l0D/EteGyC0CAwEAAaMNMAswCQYDVR0TBAIwADANBgkqhkiG9w0BAQwFAAOCAQEAqFianpqzxBvPhnp4nFYJ+9MA7y64Ugaq+47yl2q59BZKwbo/TUOgDvp/Fwf2MLBDXHQ7PKDJL4qCyokN50ZXnzNQ6IV/TYFXp1If/GcsIW/ay0SA113pqJjJhN2JbDoCeT88UKMNFy5KZV/dkkdVIsNwVr3EZwY2DGV6k/y1jMUce3DzJcoJHYZ2HD9eoMjRMdsOaZPcX4QLAxzRsbln31gA+U5HaZzbzWpx1sKhxT4jNFE2rjRkuLPBSkMH5+l6g2pW3v/bfROdrE4r6i6AYBsqOIes2yJftyWzCNhPQ/gtGC82aNgacoDv9Hfe7YzJjXh61dVgD/gZcRkOs0O3Xw==
#... cert MIIDozCCAougAwIBAgIBNjANBgkqhkiG9w0BAQwFADCBjzEnMCUGA1UEAwweU29waG9zIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MSIwIAYJKoZIhvcNAQkBFhNzb3Bob3NjYUBzb3Bob3MuY29tMRQwEgYDVQQIDAtPeGZvcmRzaGlyZTELMAkGA1UEBhMCVUsxDzANBgNVBAoMBlNvcGhvczEMMAoGA1UECwwDRVNHMB4XDTIyMDcxOTAwMDAwMFoXDTQwMDEwMTAwMDAwMFowgYcxHzAdBgNVBAMMFlNvcGhvcyBCdWlsZCBBdXRob3JpdHkxIjAgBgkqhkiG9w0BCQEWE3NvcGhvc2NhQHNvcGhvcy5jb20xFDASBgNVBAgMC094Zm9yZHNoaXJlMQswCQYDVQQGEwJVSzEPMA0GA1UECgwGU29waG9zMQwwCgYDVQQLDANFU0cwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDb/wjOJnTnTnVnqOUNeQyvCwlM9EoclTYPITK04TMAS1z8rC0qsUyvBiOh9mwtiNFH5rBe3sWdwRh5VNZRyQTass2KfbFbxhbCHppjkbYhAt2LlLdBJoXZsUxMWB81Fhe3H2hrjMNxlXBKYclyCREOoBNfpBFEvb/ixW0DYihChQFzEnONfAU20zBsl16fiNegomcfJk6zkcNpt/v181TeudReFQ7gpuSA8VHZuYsYMSntQoCEFQBweb4PqWaf/i+AKT0k265CE2VBkX4/kyDu1kgXW/quf/OJKZ+Gwh0+vchAkmSbNVOtorxN1uKny/77wU/NjRQqGm+d++wa3pEDAgMBAAGjEDAOMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggEBAA4uedsR4LLAY8keZkNRXPGOEgOObmIyfVKQB3/AgDx+12jK2YaZZd+laDLIZLFGcrp+spvxCNagSZB0L9GhG7pPcki5OYqjFe8sGQ8O80N8zW/cxlM9sqwXBqvk011Vocu6aOdh0Wj1/iQG8HTI9TgOsHgpD2xZaTiEWOQ8qeLvWl1HIq7TRKQuB2/shyv+Cw6b28iEKinEhh8ynrUub2r1ddYMpx2IOZNs7ohZJJ1QqeyObee575mpq6InPHuNGeBL6rRlgIKIER3gvkdkPtH6f7Q7YZvo6YA3uQDc05LGb/y2mrrdZ2rH5gjCs552y8i7fnSYP84yQgB7UU4lfMI=
)");
        EXPECT_CALL(mockFileSystem, openFileForRead("old.dat"))
            .WillOnce(Throw(Common::FileSystem::IFileSystemException("File doesn't exist")));
        EXPECT_CALL(mockFileSystem, openFileForRead("new.dat")).WillOnce(Return(ByMove(std::move(newData))));

        EXPECT_CALL(mockFileSystem, writeFile("added.dat", "VERSION.ini\n"));
        EXPECT_CALL(mockFileSystem, writeFile("removed.dat", ""));
        EXPECT_CALL(mockFileSystem, writeFile("changed.dat", ""));

        const auto result = Installer::ManifestDiff::manifestDiffMain(mockFileSystem, args);
        EXPECT_EQ(result, 0);
    }
} // namespace