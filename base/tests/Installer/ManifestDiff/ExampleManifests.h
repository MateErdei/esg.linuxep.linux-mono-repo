/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace
{
    const std::string one_entry(R"(".\files\base\bin\SulDownloader" 24440 2892789366e2b528b4fb5df597db166aba6fce88
#sha256 34ae939f422a460fa58581035b497e869837217fffd1e97f0e8fa36feb0715bb
#sha384 96dedb1a4a633d63301e8f7aee1f878d6a5222723316a4cb244d37577f4ccbd2f7a801adbb2292749dfc02f7270287ec
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
-----END CERTIFICATE-----)"
    );

    const std::string one_entry_changed(R"(".\files\base\bin\SulDownloader" 24440 f892789366e2b529c4fb5df597db166aba6fce88
#sha256 34ae939f422a460fa58581035b497e869837217fffd1e97f0e8fa36feb0715ff
#sha384 96dedb1a4a633d63301e8f7aee1f878d6a5222723316a4cb244d37577f4ccbd2f7a801adbb2292749dfc02f7270287ff
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
-----END CERTIFICATE-----)"
    );

    const std::string one_entry_changed_sha256(R"(".\files\base\bin\SulDownloader" 24440 2892789366e2b528b4fb5df597db166aba6fce88
#sha256 34ae939f422a460fa58581035b497e869837217fffd1e97f0e8fa36feb0715bf
#sha384 96dedb1a4a633d63301e8f7aee1f878d6a5222723316a4cb244d37577f4ccbd2f7a801adbb2292749dfc02f7270287ec
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
-----END CERTIFICATE-----)"
    );

    const std::string two_entries(R"(".\files\base\bin\SulDownloader" 24440 2892789366e2b528b4fb5df597db166aba6fce88
#sha256 34ae939f422a460fa58581035b497e869837217fffd1e97f0e8fa36feb0715bb
#sha384 96dedb1a4a633d63301e8f7aee1f878d6a5222723316a4cb244d37577f4ccbd2f7a801adbb2292749dfc02f7270287ec
".\files\base\bin\manifestdiff" 192224 8178a87bae834d7a961f08dddf322eb84d2eded7
#sha256 d17af1a75cbf6ccca2bf58e2a0c6299d1161d2f71bd0397bfcb6e78cc6f4859d
#sha384 05a4a71ffaee2a3108b518ae75f7573900604b4d01ea656cc1ce37a30cdb2bc4c41b63b08a968ccbe03dd7fde8d5df41
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
-----END CERTIFICATE-----)"
    );
}