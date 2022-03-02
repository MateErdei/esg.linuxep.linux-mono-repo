*** Settings ***
Library    ${LIBS_DIRECTORY}/LogUtils.py
Library    ${LIBS_DIRECTORY}/UpdateSchedulerHelper.py
Library    DateTime
Resource  InstallerResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../upgrade_product/UpgradeResources.robot

Test Teardown  Upgrade Test Teardown
Default Tags  INSTALLER  TAP_TESTS

*** Variables ***
${base_removed_files_manifest}              ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base/removedFiles_manifest.dat
${mtr_removed_files_manifest}               ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat
${base_files_to_delete}                     ${SOPHOS_INSTALL}/base/update/cache/primary/ServerProtectionLinux-Base/filestodelete.dat
${mtr_files_to_delete}                      ${SOPHOS_INSTALL}/base/update/cache/primary/ServerProtectionLinux-Plugin-MDR/filestodelete.dat


*** Test Cases ***
Simple Upgrade Test
    Require Fresh Install
    ${time} =  Get Current Date  exclude_millis=true
    ${message} =  Set Variable  : Reloading.
    ${result} =   Get Folder With Installer
    ${BaseDevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Copy Directory  ${result}  /opt/tmp/version2
    Replace Version  ${BaseDevVersion}   9.99.999  /opt/tmp/version2
    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Create File  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    Should Exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0
    Should Not exist  ${SOPHOS_INSTALL}/base/mcs/action/testfile
    ${BaseDevVersion2} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Should Not Be Equal As Strings  ${BaseDevVersion}  ${BaseDevVersion2}

    Check Expected Base Processes Are Running

    Should Not Have A Given Message In Journalctl Since Certain Time  ${message}  ${time}
    Should Have Set KillMode To Mixed

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Simple Downgrade Test
    Require Fresh Install
    ${distribution} =   Get Folder With Installer
    Create File  ${SOPHOS_INSTALL}/bin/blah
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy
    Directory Should Not Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    ${result} =  Run Process   ${SOPHOS_INSTALL}/bin/uninstall.sh  --downgrade  --force

    Should Be Equal As Integers    ${result.rc}    0
    Directory Should Exist   ${SOPHOS_INSTALL}/logs/base/downgrade-backup
    Should not exist  ${SOPHOS_INSTALL}/bin/blah
    Should not exist  ${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy

    ${BaseDevVersion} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Copy Directory  ${distribution}  /opt/tmp/version2
    Replace Version  ${BaseDevVersion}   0.1.1  /opt/tmp/version2

    ${result} =  Run Process  chmod  +x  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0

    ${result} =  Run Process  /opt/tmp/version2/install.sh
    Should Be Equal As Integers    ${result.rc}    0

    ${BaseDevVersion2} =     Get Version Number From Ini File   ${SOPHOS_INSTALL}/base/VERSION.ini
    Should Not Be Equal As Strings  ${BaseDevVersion}  ${BaseDevVersion2}

    Check Expected Base Processes Are Running

    Mark Expected Critical In Log  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log  mcsrouter.mcs <> Not registered: MCSID is not present
    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/watchdog.log  ProcessMonitoringImpl <> /opt/sophos-spl/base/bin/mcsrouter died with 1

    Check All Product Logs Do Not Contain Error
    Check All Product Logs Do Not Contain Critical

Check Local Logger Config Settings Are Processed and Persist After Upgrade
    ${expected_local_file_contents} =  Set Variable  [global]\nVERBOSITY = DEBUG\n

    Require Fresh Install
    Check Expected Base Processes Are Running

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Log Contains  configured for level: INFO

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check mcsrouter Log Contains  Logging level: 20

    Override Local LogConf File for a component   DEBUG  global
    Restart And Remove MCS And Watchdog Logs

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Watchdog Log Contains  configured for level: DEBUG

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check mcsrouter Log Contains  Logging level: 10

    Run Full Installer

    ${actual_local_file_contents} =  Get File  ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    Should Be Equal As Strings  ${expected_local_file_contents}  ${actual_local_file_contents}

VersionCopy File in the Wrong Location Is Removed
    [Tags]  INSTALLER
    Combine MTR 0-6-0 Component Suite
    Combine MTR Develop Component Suite
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-0-6-0/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin-060/install.sh

    #fake the file being copied to the wrong location
    Create Directory   ${SOPHOS_INSTALL}/opt/sophos-spl/base/bin
    Create File   ${SOPHOS_INSTALL}/opt/sophos-spl/base/bin/versionedcopy.0
    ${result} =  Run Process  ln  -sf  ${SOPHOS_INSTALL}/opt/sophos-spl/base/bin/versionedcopy.0  ${SOPHOS_INSTALL}/opt/sophos-spl/base/bin/versionedcopy
    Should Be Equal As Integers     ${result.rc}    0

    # replace version files to prove installer updated product as expected.

    Create File  ${InstalledBaseVersionFile}        PRODUCT_VERSION = 0.0.0.9999
    Create File  ${InstalledMDRPluginVersionFile}   PRODUCT_VERSION = 0.0.0.999

    Log File  ${InstalledBaseVersionFile}
    Log FIle  ${InstalledMDRPluginVersionFile}

    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}

    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin/install.sh

    ${BaseDevVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =      Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Directory Should Not Exist   ${SOPHOS_INSTALL}/opt/
    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}

Verify Upgrading Will Remove Files Which Are No Longer Required
    [Tags]      INSTALLER
    Combine MTR 0-6-0 Component Suite
    Combine MTR Develop Component Suite
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-0-6-0/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin-060/install.sh
    Check Files Before Upgrade
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin/install.sh
    Check Files After Upgrade

Verify Upgrading Will Not Remove Files Which Are Outside Of The Product Realm
    [Tags]  INSTALLER
    Combine MTR 0-6-0 Component Suite
    Combine MTR Develop Component Suite
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-0-6-0/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin-060/install.sh
    Move File   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base-component/manifest.dat  /tmp/base-manifest.dat
    Move File  ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat  /tmp/MDR-manifest.dat

    Move File  /tmp/MDR-manifest.dat    ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base-component/manifest.dat
    Move File  /tmp/base-manifest.dat   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat

    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin/install.sh

    # ensure that the list of files to remove contains files which are outside of the components realm
    ${BASE_REMOVE_FILE_CONTENT} =  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    Should Contain  ${BASE_REMOVE_FILE_CONTENT}  plugins/mtr

    ${MTR_REMOVE_FILE_CONTENT} =  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat
    Should Contain  ${MTR_REMOVE_FILE_CONTENT}  base

    # ensure that the cleanup process is prevented from deleting files which are not stored in the component realm
    # note files listed in the components filestodelete.dat file, will be deleted by that compoent, so these files need to be
    # filtered out.

    Check Files Have Not Been Removed  ${SOPHOS_INSTALL}  ${base_removed_files_manifest}  plugins/mtr  ${mtr_files_to_delete}
    Check Files Have Not Been Removed  ${SOPHOS_INSTALL}  ${mtr_removed_files_manifest}  base   ${base_files_to_delete}

Version Copy Versions All Changed Files When Upgrading
    [Tags]  INSTALLER
    Combine MTR 0-6-0 Component Suite
    Combine MTR Develop Component Suite
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base-0-6-0/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin-060/install.sh

    ${BaseReleaseVersion}=  Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrReleaseVersion}=  Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}

    ${BaseManifestPath}=  Set Variable  ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base-component/manifest.dat
    ${MTRPluginManifestPath}=  Set Variable  ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat

    ${BeforeManifestBase}=  Get File  ${BaseManifestPath}
    ${BeforeManifestPluginMdr}=  Get File  ${MTRPluginManifestPath}

    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-base/install.sh
    Run Specific Installer Directly   ${SYSTEMPRODUCT_TEST_INPUT}/sspl-mdr-control-plugin/install.sh


    ${BaseDevVersion} =  Get Version Number From Ini File   ${InstalledBaseVersionFile}
    ${MtrDevVersion} =  Get Version Number From Ini File   ${InstalledMDRPluginVersionFile}
    Should Not Be Equal As Strings  ${BaseReleaseVersion}  ${BaseDevVersion}
    Should Not Be Equal As Strings  ${MtrReleaseVersion}  ${MtrDevVersion}

    Check Files In Versioned Copy Manifests Have Correct Symlink Versioning

    ${AfterManifestBase}=  Get File  ${BaseManifestPath}
    ${AfterManifestPluginMdr}=  Get File  ${MTRPluginManifestPath}

    ${ChangedBase}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat
    ${AddedBase}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    ${combinedBaseChanges}=  Catenate  SEPARATOR=\n  ${ChangedBase}  ${AddedBase}
    ${ChangedPluginMdr}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/changedFiles_manifest.dat
    ${AddedPluginMdr}=  Get File  ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/addedFiles_manifest.dat
    ${combinedPluginMdrChanges}=  Catenate  SEPARATOR=\n  ${ChangedPluginMdr}  ${AddedPluginMdr}

    Compare Before And After Manifests With Changed Files Manifest  ${BeforeManifestBase}       ${AfterManifestBase}        ${combinedBaseChanges}
    Compare Before And After Manifests With Changed Files Manifest  ${BeforeManifestPluginMdr}  ${AfterManifestPluginMdr}   ${combinedPluginMdrChanges}


*** Keywords ***

Restart And Remove MCS And Watchdog Logs
    Run Shell Process  systemctl stop sophos-spl    OnError=failed to restart sophos-spl
    Remove File  ${SOPHOS_INSTALL}/logs/base/watchdog.log
    Remove File  ${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log
    Run Shell Process  systemctl start sophos-spl   OnError=failed to restart sophos-spl

Upgrade Test Teardown
    LogUtils.Dump Log  ${SOPHOS_INSTALL}/base/etc/logger.conf
    LogUtils.Dump Log  ${SOPHOS_INSTALL}/base/etc/logger.conf.local
    General Test Teardown
    Remove Directory  /opt/tmp/version2  true
    Require Uninstalled

Check Files Before Upgrade
    # This is a selection of files from Base product, based on the version initialy installed
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999.0
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999
    File Should Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999.0

    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999.0
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999
    File Should Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999.0

Check Files After Upgrade
    # This is a selection of removed files from Base product, based on the version initialy installed
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/fake_lib.so.1.66.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999
    File Should Not Exist   ${SOPHOS_INSTALL}/base/lib64/faker_lib.so.2.23.999.0

    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/also_a_fake_lib.so.5.86.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/fake_lib.so.1.66.999.0
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999
    File Should Not Exist   ${SOPHOS_INSTALL}/plugins/mtr/lib64/faker_lib.so.2.23.999.0

    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/addedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/changedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Base-component/removedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base-component/manifest.dat

    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/addedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/changedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/tmp/ServerProtectionLinux-Plugin-MDR/removedFiles_manifest.dat
    File Should Exist   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Plugin-MDR/manifest.dat

    File Should Exist   ${SOPHOS_INSTALL}/base/update/ServerProtectionLinux-Base-component/manifest.dat
