// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/OSUtilitiesImpl/CloudMetadataConverters.h"
#include "Common/OSUtilitiesImpl/PlatformUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/OSUtilitiesImpl/MockILocalIP.h"

using namespace Common::OSUtilitiesImpl;

TEST(TestPlatformUtils, PopulateVendorDetailsForUbuntu)
{
    std::vector<std::string> osReleaseContents = { "NAME=Ubuntu",
                                                    "VERSION_ID=18.04",
                                                    "VERSION_CODENAME=bionic",
                                                    "PRETTY_NAME=\"Ubuntu 18.04.6 LTS\"" };

    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile("/etc/os-release")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readLines("/etc/os-release")).WillRepeatedly(Return(osReleaseContents));
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    PlatformUtils platformUtils;
    ASSERT_EQ(platformUtils.getVendor(), "ubuntu");
    ASSERT_EQ(platformUtils.getOsName(), "Ubuntu 18.04.6 LTS");
    ASSERT_EQ(platformUtils.getOsMajorVersion(), "18");
    ASSERT_EQ(platformUtils.getOsMinorVersion(), "04");

    scopedReplaceFileSystem.reset();
}

TEST(TestPlatformUtils, GetAndConvertAwsMetadataToXml)
{
    std::string responseBody = R"({
        "accountId" : "123456789",
        "architecture" : "x86_64",
        "availabilityZone" : "eu-west-1c",
        "billingProducts" : "null",
        "devpayProductCodes" : "null",
        "marketplaceProductCodes" : [ "mockProductCode" ],
        "imageId" : "mock-ami",
        "instanceId" : "mockInstanceId",
        "instanceType" : "c4.large",
        "kernelId" : "null",
        "pendingTime" : "2022-04-11T15:06:21Z",
        "privateIp" : "123.45.67.891",
        "ramdiskId" : "null",
        "region" : "eu-west-1",
        "version" : "2017-09-30"
    })";

    std::string result = CloudMetadataConverters::parseAwsMetadataJson(responseBody);
    Common::XmlUtilities::AttributesMap resultXml = Common::XmlUtilities::parseXml(result);

    ASSERT_EQ(resultXml.lookup("aws/region").contents(), "eu-west-1");
    ASSERT_EQ(resultXml.lookup("aws/accountId").contents(), "123456789");
    ASSERT_EQ(resultXml.lookup("aws/instanceId").contents(), "mockInstanceId");
}

TEST(TestPlatformUtils, GetAndConvertAzureMetadataToXml)
{
    std::string responseBody = R"({
        "compute": {
            "azEnvironment": "AZUREPUBLICCLOUD",
            "extendedLocation": {
                "type": "edgeZone",
                "name": "microsoftlosangeles"
            },
            "evictionPolicy": "",
            "isHostCompatibilityLayerVm": "true",
            "licenseType":  "",
            "location": "westus",
            "name": "examplevmname",
            "offer": "UbuntuServer",
            "osProfile": {
                "adminUsername": "admin",
                "computerName": "examplevmname",
                "disablePasswordAuthentication": "true"
            },
            "osType": "Linux",
            "placementGroupId": "f67c14ab-e92c-408c-ae2d-da15866ec79a",
            "plan": {
                "name": "planName",
                "product": "planProduct",
                "publisher": "planPublisher"
            },
            "platformFaultDomain": "36",
            "platformSubFaultDomain": "",
            "platformUpdateDomain": "42",
            "priority": "Regular",
            "publicKeys": [{
                    "keyData": "ssh-rsa 0",
                    "path": "/home/user/.ssh/authorized_keys0"
                },
                {
                    "keyData": "ssh-rsa 1",
                    "path": "/home/user/.ssh/authorized_keys1"
                }
            ],
            "publisher": "Canonical",
            "resourceGroupName": "macikgo-test-may-23",
            "resourceId": "/subscriptions/xxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx/resourceGroups/macikgo-test-may-23/providers/Microsoft.Compute/virtualMachines/examplevmname",
            "securityProfile": {
                "secureBootEnabled": "true",
                "virtualTpmEnabled": "false"
            },
            "sku": "18.04-LTS",
            "storageProfile": {
                "dataDisks": [{
                    "bytesPerSecondThrottle": "979202048",
                    "caching": "None",
                    "createOption": "Empty",
                    "diskCapacityBytes": "274877906944",
                    "diskSizeGB": "1024",
                    "image": {
                      "uri": ""
                    },
                    "isSharedDisk": "false",
                    "isUltraDisk": "true",
                    "lun": "0",
                    "managedDisk": {
                      "id": "/subscriptions/xxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx/resourceGroups/macikgo-test-may-23/providers/Microsoft.Compute/disks/exampledatadiskname",
                      "storageAccountType": "StandardSSD_LRS"
                    },
                    "name": "exampledatadiskname",
                    "opsPerSecondThrottle": "65280",
                    "vhd": {
                      "uri": ""
                    },
                    "writeAcceleratorEnabled": "false"
                }],
                "imageReference": {
                    "id": "",
                    "offer": "UbuntuServer",
                    "publisher": "Canonical",
                    "sku": "16.04.0-LTS",
                    "version": "latest"
                },
                "osDisk": {
                    "caching": "ReadWrite",
                    "createOption": "FromImage",
                    "diskSizeGB": "30",
                    "diffDiskSettings": {
                        "option": "Local"
                    },
                    "encryptionSettings": {
                        "enabled": "false"
                    },
                    "image": {
                        "uri": ""
                    },
                    "managedDisk": {
                        "id": "/subscriptions/xxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx/resourceGroups/macikgo-test-may-23/providers/Microsoft.Compute/disks/exampleosdiskname",
                        "storageAccountType": "StandardSSD_LRS"
                    },
                    "name": "exampleosdiskname",
                    "osType": "Linux",
                    "vhd": {
                        "uri": ""
                    },
                    "writeAcceleratorEnabled": "false"
                },
                "resourceDisk": {
                    "size": "4096"
                }
            },
            "subscriptionId": "xxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx",
            "tags": "baz:bash;foo:bar",
            "version": "15.05.22",
            "virtualMachineScaleSet": {
                "id": "/subscriptions/xxxxxxxx-xxxxx-xxx-xxx-xxxx/resourceGroups/resource-group-name/providers/Microsoft.Compute/virtualMachineScaleSets/virtual-machine-scale-set-name"
            },
            "vmId": "02aab8a4-74ef-476e-8182-f6d2ba4166a6",
            "vmScaleSetName": "crpteste9vflji9",
            "vmSize": "Standard_A3",
            "zone": ""
        },
        "network": {
            "interface": [{
                "ipv4": {
                   "ipAddress": [{
                        "privateIpAddress": "10.144.133.132",
                        "publicIpAddress": ""
                    }],
                    "subnet": [{
                        "address": "10.144.133.128",
                        "prefix": "26"
                    }]
                },
                "ipv6": {
                    "ipAddress": []
                },
                "macAddress": "0011AAFFBB22"
            }]
        }
    })";

    std::string result = CloudMetadataConverters::parseAzureMetadataJson(responseBody);
    Common::XmlUtilities::AttributesMap resultXml = Common::XmlUtilities::parseXml(result);

    ASSERT_EQ(resultXml.lookup("azure/vmId").contents(), "02aab8a4-74ef-476e-8182-f6d2ba4166a6");
    ASSERT_EQ(resultXml.lookup("azure/vmName").contents(), "examplevmname");
    ASSERT_EQ(resultXml.lookup("azure/resourceGroupName").contents(), "macikgo-test-may-23");
    ASSERT_EQ(resultXml.lookup("azure/subscriptionId").contents(), "xxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx");
}

TEST(TestPlatformUtils, GetAndConvertGcpMetadataToXml)
{
    std::map<std::string, std::string> metadataValues{ { "id", "mock-id" },
                                                       { "zone", "mock-zone" },
                                                       { "hostname", "mock-hostname" } };

    std::string result = CloudMetadataConverters::parseGcpMetadata(metadataValues);
    Common::XmlUtilities::AttributesMap resultXml = Common::XmlUtilities::parseXml(result);

    ASSERT_EQ(resultXml.lookup("google/hostname").contents(), metadataValues["hostname"]);
    ASSERT_EQ(resultXml.lookup("google/id").contents(), metadataValues["id"]);
    ASSERT_EQ(resultXml.lookup("google/zone").contents(), metadataValues["zone"]);
}

TEST(TestPlatformUtils, GetAndConvertOracleMetadataToXml)
{
    std::string responseBody = R"({
        "availabilityDomain" : "EMIr:PHX-AD-1",
        "faultDomain" : "FAULT-DOMAIN-3",
        "compartmentId" : "ocid1.tenancy.oc1..exampleuniqueID",
        "displayName" : "my-example-instance",
        "hostname" : "my-hostname",
        "id" : "ocid1.instance.oc1.phx.exampleuniqueID",
        "image" : "ocid1.image.oc1.phx.exampleuniqueID",
        "metadata" : {
          "ssh_authorized_keys" : "example-ssh-key"
        },
        "region" : "phx",
        "canonicalRegionName" : "us-phoenix-1",
        "ociAdName" : "phx-ad-1",
        "regionInfo" : {
          "realmKey" : "oc1",
          "realmDomainComponent" : "oraclecloud.com",
          "regionKey" : "PHX",
          "regionIdentifier" : "us-phoenix-1"
        },
        "shape" : "VM.Standard.E3.Flex",
        "state" : "Running",
        "timeCreated" : 1600381928581,
        "agentConfig" : {
            "monitoringDisabled" : false,
            "managementDisabled" : false,
            "allPluginsDisabled" : false,
            "pluginsConfig" : [ {
            "name" : "OS Management Service Agent",
            "desiredState" : "ENABLED"
            }, {
            "name" : "Custom Logs Monitoring",
            "desiredState" : "ENABLED"
            }, {
            "name" : "Compute Instance Run Command",
            "desiredState" : "ENABLED"
            }, {
            "name" : "Compute Instance Monitoring",
            "desiredState" : "ENABLED"
            } ]
        },
        "freeformTags": {
            "Department": "Finance"
        },
        "definedTags": {
          "Operations": {
            "CostCenter": "42"
          }
        }
    })";

    std::string result = CloudMetadataConverters::parseOracleMetadataJson(responseBody);
    Common::XmlUtilities::AttributesMap resultXml = Common::XmlUtilities::parseXml(result);

    ASSERT_EQ(resultXml.lookup("oracle/region").contents(), "phx");
    ASSERT_EQ(resultXml.lookup("oracle/availabilityDomain").contents(), "EMIr:PHX-AD-1");
    ASSERT_EQ(resultXml.lookup("oracle/compartmentId").contents(), "ocid1.tenancy.oc1..exampleuniqueID");
    ASSERT_EQ(resultXml.lookup("oracle/displayName").contents(), "my-example-instance");
    ASSERT_EQ(resultXml.lookup("oracle/hostname").contents(), "my-hostname");
    ASSERT_EQ(resultXml.lookup("oracle/state").contents(), "Running");
    ASSERT_EQ(resultXml.lookup("oracle/instanceId").contents(), "ocid1.instance.oc1.phx.exampleuniqueID");
}

TEST(TestPlatformUtils, GetAndCorrectlySortIpAddressesForStatusXML)
{
    PlatformUtils platformUtils;

    auto mockLocalIP = std::make_shared<StrictMock<MockILocalIP>>();
    std::vector<Common::OSUtilities::Interface> interfaces = MockILocalIP::buildInterfacesHelper();
    platformUtils.sortInterfaces(interfaces);

    std::vector<std::string> ip4Addresses = platformUtils.getIp4Addresses(interfaces);
    std::vector<std::string> ip6Addresses = platformUtils.getIp6Addresses(interfaces);

    std::vector<std::string> expectedSortedIp4Addresses = {
        "192.168.168.64", "125.184.182.125", "192.240.35.35", "172.6.251.34", "100.80.13.214"
    };
    std::vector<std::string> expectedSortedIp6Addresses = { "f5eb:f7a4:323f:e898:f212:d0a6:3405:feec",
                                                            "4d48:b9eb:62a5:4119:6e7f:89e9:a652:4941",
                                                            "e36:e861:82a2:a473:2c24:7a07:7867:6a50",
                                                            "8cc1:bec7:87c5:b668:62bf:ccaa:9f56:8f7f",
                                                            "edbd:3bb8:bffb:4b6b:3536:2be7:3066:4276" };

    ASSERT_EQ(ip4Addresses, expectedSortedIp4Addresses);
    ASSERT_EQ(ip6Addresses, expectedSortedIp6Addresses);
    ASSERT_EQ(platformUtils.getFirstIpAddress(ip4Addresses), "192.168.168.64");
    ASSERT_EQ(platformUtils.getFirstIpAddress(ip6Addresses), "f5eb:f7a4:323f:e898:f212:d0a6:3405:feec");
}

TEST(TestPlatformUtils, getAzureMetadataHandlesUnexpected200Responses)
{
    PlatformUtils platformUtils;
    Common::HttpRequests::Response garbageResponse;
    garbageResponse.body = "garbage, not json";
    garbageResponse.errorCode = Common::HttpRequests::OK;
    garbageResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    EXPECT_CALL(*httpRequester, post(_)).WillRepeatedly(Return(garbageResponse));
    EXPECT_CALL(*httpRequester, put(_)).WillRepeatedly(Return(garbageResponse));
    EXPECT_CALL(*httpRequester, get(_)).WillRepeatedly(Return(garbageResponse));
    std::string response = "unset value";
    EXPECT_NO_THROW(response = platformUtils.getCloudPlatformMetadata(httpRequester));
    EXPECT_EQ(response, "");
}

TEST(CloudMetadataConverters, verifyGoogleId)
{
    EXPECT_TRUE(CloudMetadataConverters::verifyGoogleId("abc"));
    EXPECT_TRUE(CloudMetadataConverters::verifyGoogleId("ABC"));
    EXPECT_TRUE(CloudMetadataConverters::verifyGoogleId("aAbBcC"));
    EXPECT_TRUE(CloudMetadataConverters::verifyGoogleId("aA1bB2cC3"));
    EXPECT_TRUE(CloudMetadataConverters::verifyGoogleId("a-A-1-b-B-2-c-C-3"));

    EXPECT_FALSE(CloudMetadataConverters::verifyGoogleId("bad id with spaces"));
    EXPECT_FALSE(CloudMetadataConverters::verifyGoogleId("BadIDWithSome<AngleBrackets>"));
}
