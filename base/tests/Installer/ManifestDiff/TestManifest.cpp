/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Installer/ManifestDiff/Manifest.h>

#include "ExampleManifests.h"

#include <gtest/gtest.h>

namespace
{
    class TestManifest
            : public ::testing::Test
    {
    public:
    };

    Installer::ManifestDiff::Manifest manifestFromString(const std::string& s)
    {
        std::stringstream ost(s);
        return Installer::ManifestDiff::Manifest(ost);
    }
}

TEST_F(TestManifest, TestConstructionFromEmptyStream) //NOLINT
{
    std::stringstream ist("");

    EXPECT_NO_THROW(Installer::ManifestDiff::Manifest manifest(ist)); //NOLINT
}

TEST_F(TestManifest, TestConstructionFromStream) //NOLINT
{
    std::string example(R"(".\files\base\bin\SulDownloader" 24440 2892789366e2b528b4fb5df597db166aba6fce88
#sha256 34ae939f422a460fa58581035b497e869837217fffd1e97f0e8fa36feb0715bb
#sha384 96dedb1a4a633d63301e8f7aee1f878d6a5222723316a4cb244d37577f4ccbd2f7a801adbb2292749dfc02f7270287ec
".\files\base\bin\manifestdiff" 192224 8178a87bae834d7a961f08dddf322eb84d2eded7
#sha256 d17af1a75cbf6ccca2bf58e2a0c6299d1161d2f71bd0397bfcb6e78cc6f4859d
#sha384 05a4a71ffaee2a3108b518ae75f7573900604b4d01ea656cc1ce37a30cdb2bc4c41b63b08a968ccbe03dd7fde8d5df41
".\files\base\bin\mcsrouter" 485 4a8dcda8677b03cb567153660e7f7915e62d2531
#sha256 eb601a725b5494cfe6bc5081f1582d4071d60e1439990fb0836bcc35d208e055
#sha384 9a7546dd6255c262c45e5cc85a237387ec09bd9b2f9733f8a576377fa4a4fcbbbcc6c9b96aebd60ee8442b15800b689d
".\files\base\bin\registerCentral" 401 6c805f615edeb4e96fdb47b91fc22ab2c370a374
#sha256 7341bb5124c888413686becb9703a1b03121491985ac76dbe3e2c34ff392fd0a
#sha384 6c1a2e7234daba8aff6e4fec0414a5f83a31a5aa34188c00c4a04c9a49ee4f52a8559fbfa02ea6df14039125b58d5770
".\files\base\bin\sophos_managementagent" 91648 b1a3b86e872c2de8cfc6d45c66f16adeebd174b7
#sha256 764dda0c04b1b6ecd64da74ef0b7392a15ac73754a9adcd2b4b14c0312756e86
#sha384 46397a65edc59f8355bbfeafa0e688fca6dc23e9d336d2ada69abe6ef0bb6402ef793a7a7750a950347e251f8bdf9de0
".\files\base\bin\sophos_watchdog" 933704 b18a5e8333a0692e343e1854ee29fd1ccda674bb
#sha256 297c76ac6f535f3e41ee7dc190f00412d3e72c2e2597d56e43d3c33e4b16120f
#sha384 467612eac4e6ac894fe9c6fab82dfb5c827a65dd1ee5e93aa0690e2de198af6035871085e261b0ffe4d0965aabab672a
".\files\base\bin\uninstall.sh" 1645 f7ebcbe37597a40cd015708d9b60b9f0362586a3
#sha256 b8723cbfe6de7747434f0dd7742976c3ba6f1ae82cd104643863205154258e1e
#sha384 037aa6c6bf639552efa83898e713382f0af660a18887acc05f2fb117c77500d40eabad817a6046e75973ff15c562a119
".\files\base\bin\versionedcopy" 286392 f100bb6f6db4ac08a10dad7f2be86eeb9d09be3d
#sha256 151ca2afbd667a572820e03351f42edb576f2021b6f336dac622ec60544f904b
#sha384 b8c6989ae985d9e42841be47fd1493c165055909cd85336b2a88144bd505d4c01db15f3a6cdf07bd99ced97438048b91
".\files\base\lib64\libSUL.so.0.0.0" 14068701 2f981ce52ac529901f6028ccdf4af112225df0da
#sha256 a32ad426051c85f3014a9e3a5cec86f992997939349f9308bd7fae60f6768852
#sha384 f17bd3451c7f8fb89b80b479c4ed05da4fe85d8174194436d60b91d1f1933d49ed66f9efb4c73689aa4158f2a4e0e79e
".\files\base\lib64\libaesclib.so.0.0.0" 77481 7de66f046a73092d7a7af7a70388cda2d0d1c255
#sha256 5369efafe1a75d02f8be12e8172fec8964059dfce960d29474a1ac5e62846255
#sha384 42837547508820c2f97d7c5f740186ff019470aa2e3c9aa121c9b0b0b0b6e5e18e52dd47429390c3dfd67b63d84d17ce
".\files\base\lib64\libapplicationconfigurationimpl.so" 402800 df731fdeb2f8e9d839c9651b1384d85a486b848a
#sha256 9897a7c0da620c8296bddce508727112f705379bc0f3931a162d5ff03fd86f40
#sha384 f9cb52756a28fa2e9259d3a43a6bcf675d4e71607e0b1e6b2de356d9a2036f782b3187da489a3f35fcb6fcab10b3cd62
".\files\base\lib64\libboost_atomic.so.1.66.0" 18320 c115280fb99acec1cd4514c17acb9bee4a024c4d
#sha256 8d6b42a5e83d05c0cba5df39d48608ce31387242fbf81378499b45944efd64dd
#sha384 34bca5ce2bd0a59777a21bee08bc6c40ab493f195170a1d4728a77100d52d8e98d87899b56ce55a5086eb158ed916e51
".\files\base\lib64\libboost_chrono.so.1.66.0" 383512 64ba6c6688573654b8ffc23e1c58fee4af63379b
#sha256 6f32726c5dc0f442fd1d9aff067231e26bc63024fac79d9e032afaf475bd7339
#sha384 11f5f04d0f596d696feb6a1edd70ffd4876c402e9ab22da6e455bf470b58c07fc6a73af16810f801465b735e0931e70d
".\files\base\lib64\libboost_date_time.so.1.66.0" 583808 edc698edae5a171b4cdd9e6e65d92d20d1807fef
#sha256 c67faeca4ba354a5f6088fc51efb0af98b92a5371dc24cd871c90f8830f8de26
#sha384 10079ade7cbd6d2eedf05c507a8f4382e79331ae40d2f71fb3039c7b104997f436d39ac82a86fbfa30bd05b1bd3a1e92
".\files\base\lib64\libboost_filesystem.so.1.66.0" 1272680 74a22107489d46eb0e50a2c57af4427f47f4cb96
#sha256 dbf4e4062960f53f67f1e98a1a29bbae6d14d557a19ccc3d3b14a697c738adfc
#sha384 1a985e10e4aa699569fa869fc23dd0d8aad7067340c2085c2190650f2551279e2986c8460bbff4f88f4b29a2bab4f29d
".\files\base\lib64\libboost_regex.so.1.66.0" 9000408 028dc3743308245573a028865d3a2c4856f95a94
#sha256 73414cab6158db2663bfed268a85e65878febffce9c563404f7478093154c6cb
#sha384 5f37f370376480c7d7b16848246837c3aa0fbf00ef257414ca9022974af2d31993d5501e5fd01d40c408bdb0962f4324
-----BEGIN SIGNATURE-----
y+ojoUCIr2aNv8nDv1F+HQgC+CH9K3iVeodCk4OdFSHyHEg+/NmGLAOy6IUwlZ0v
w6AFkJBxKeacIXjoV87oMeL9XJ0doKM1gMloUJpeJ0kgzN8VD5P0sad8KbPag2RE
8HR9uWY+VZ3+KXvOWVtk7zUnMiSho+7x3tRZGvoTDM0Oc/GmxnRzzDbrjtwquS0T
TRV8dXnl6UZLA2oqGpc6Lw==
-----END SIGNATURE-----
-----BEGIN CERTIFICATE-----
MIIDSjCCAjKgAwIBAgIBCTANBgkqhkiG9w0BAQUFADCBljEfMB0GA1UEAwwWU29w
aG9zIEJ1aWxkIEF1dGhvcml0eTElMCMGCSqGSIb3DQEJARYWc29waG9zYnVpbGRA
c29waG9zLmNvbTEUMBIGA1UECAwLT3hmb3Jkc2hpcmUxCzAJBgNVBAYTAlVLMRMw
EQYDVQQKDApTb3Bob3MgUGxjMRQwEgYDVQQLDAtEZXZlbG9wbWVudDAiGA8yMDA0
MDEwMTAwMDAwMFoYDzIwMTgxMjIwMDAwMDAwWjCBijEUMBIGA1UEAwwLRGFpbHkg
QnVpbGQxJDAiBgkqhkiG9w0BCQEWFURhaWx5QnVpbGRAc29waG9zLmNvbTEUMBIG
A1UECAwLT3hmb3Jkc2hpcmUxCzAJBgNVBAYTAlVLMRMwEQYDVQQKDApTb3Bob3Mg
UGxjMRQwEgYDVQQLDAtEZXZlbG9wbWVudDCBvzANBgkqhkiG9w0BAQEFAAOBrQAw
gakCgaEA3F2HFDU3zZ2qVdxCeWlbVRmQViytQuCSkzU8D0XF4p4flFeZ4JaOO2Q2
q7rQS9QvSV0fmID//BnUiT3+69asvqNWtDwwAAIjbziKFvpbR+MMKi9aqalpuDz5
s6MUUALFuUKL1LVqPITFvSqaZYkQ/pMtJ88XIKcINGW2VEHD5W1qJR27RiIB9zir
G3LvOS2R+ZMUqwkH/mXb7GEKpWabFQIDAQABow0wCzAJBgNVHRMEAjAAMA0GCSqG
SIb3DQEBBQUAA4IBAQABsNis2ytQ3XckqhWfN/WWGLA1sl9hxuBFtOyn0XTA7FHj
s59x/FXXJYYiTpIJfZlMwgnqpuElF3YLeXAN09h+9/q1SIbx3sYu22ktGSc4giq6
UGM1fArWPUkca/ihCL2aE2UW+lziDj3lMuoMEp8wtcUHKa6LhZ0f8hwFwS8T2JOu
vhskeQr+pn7unHNSbWzv8iNAXbqSFWp+7cCysaLn6ey7V5fcu+ymPgREMf5XwzuA
VIrEH+AhEUc8vQIQ/27HK0fgpbxKjpE5on233GUWe/IrazGHuQxuYbxwteC9whds
YKeb0xkxnpobTKInUKtzFbN3TjwkJ9XifsBHpFYq
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDrDCCApSgAwIBAgIBCDANBgkqhkiG9w0BAQUFADCBhTEnMCUGA1UEAxMeU29w
aG9zIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MSIwIAYJKoZIhvcNAQkBFhNzb3Bo
b3NjYUBzb3Bob3MuY29tMRQwEgYDVQQIEwtPeGZvcmRzaGlyZTELMAkGA1UEBhMC
VUsxEzARBgNVBAoTClNvcGhvcyBQbGMwIhgPMjAwNDAxMDEwMDAwMDBaGA8yMDE4
MTIyMDAwMDAwMFowgZYxHzAdBgNVBAMMFlNvcGhvcyBCdWlsZCBBdXRob3JpdHkx
JTAjBgkqhkiG9w0BCQEWFnNvcGhvc2J1aWxkQHNvcGhvcy5jb20xFDASBgNVBAgM
C094Zm9yZHNoaXJlMQswCQYDVQQGEwJVSzETMBEGA1UECgwKU29waG9zIFBsYzEU
MBIGA1UECwwLRGV2ZWxvcG1lbnQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQDFyYYx6NdJjxiIW5PfxccYsvo2T/+80yJE7+yfXid0YikVSZX7vLO8ATB3
+fv+bgy+7dr5G9DnHTD1Cmmkc2DdxzTuhAbZQFP9VjcZFK8iKn9cKPgim67GiGz2
/MY4309GiqYcosG/slugaO7t80VagRFqiupU+unJ+hqW5vFhhg0BLE3P4hnOSiku
wQNRD05jrUx+CWjG6vjIlYwfDFM1R6+/MbFIfF2hGrjrNYAt358OBmGphn8U6Y66
29u2MvcUmJwX2A6yqO3l+qs94hmP5/wtEH6joR5Nn8+80sSamXc8fkxp6bQJ1x5F
xEwpSbuv+K8ePJjREHFtdFaxvR7HAgMBAAGjEDAOMAwGA1UdEwQFMAMBAf8wDQYJ
KoZIhvcNAQEFBQADggEBADv3qX4TCXm68gMPEuDtkDjw47p6Tc4Gioqrn0AGmDoX
dlYn9Txr+qyF8n7jnEe5soGSt2CGNDMRbuLeSOd0RrO9FsThcrxiYe0kE8zOvTv5
RBXakm9cTcyRTKtbT95A2oT7QuqkgB2Qiw2EV+2vL25wHFPEObtb6FQfV1HDIDbX
h25DaVH3ryjxz21LDFdckLKbHlXoKw5UlPZ57hZ6n/qgAAcwJlINoKGZLs75XAmT
wcFHdBmgXDsTn+DjOa1AHcLXOo9pb1LBJa1/+ozQrRFCEZHeNmUDBCg6fx2EUfg/
sP1Z9XZZl5f7fOerDon/kkEfmaWjmHDGuRotVKq2STU=
-----END CERTIFICATE-----
)");

    Installer::ManifestDiff::Manifest manifest(manifestFromString(example));
    EXPECT_EQ(manifest.size(),16);
}

TEST_F(TestManifest, TestWorkingOutAddition) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(one_entry));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(two_entries));

    Installer::ManifestDiff::ManifestEntrySet added(new_manifest.calculateAdded(old_manifest));
    ASSERT_EQ(added.size(),1);

    EXPECT_EQ(added.begin()->path(),"files/base/bin/manifestdiff");
}


TEST_F(TestManifest, TestWorkingOutRemoval) //NOLINT
{
    Installer::ManifestDiff::Manifest old_manifest(manifestFromString(two_entries));
    Installer::ManifestDiff::Manifest new_manifest(manifestFromString(one_entry));

    Installer::ManifestDiff::ManifestEntrySet removed(new_manifest.calculateRemoved(old_manifest));
    ASSERT_EQ(removed.size(),1);
    EXPECT_EQ(removed.begin()->path(),"files/base/bin/manifestdiff");
}
