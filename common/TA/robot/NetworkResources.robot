*** Settings ***
Library         Process

Library         ${COMMON_TEST_LIBS}/PolicyUtils.py

*** Variables ***
${TEST_IPADDR}                          1.2.3.0/24


*** Keywords ***
#Iptable
Add Iptable Rule From IP
    [Arguments]    ${ruletype}=ACCEPT    ${ip}=${TEST_IPADDR}    ${table}=INPUT
    Run Shell Process   iptables -A ${table} -s ${ip} -j ${ruletype}   OnError=Failed add ${ip} ${ruletype} rule to ${table} table
    Check Iptable Rule Exists     ${ip}


Add Iptable Rule From URL
    [Arguments]    ${url}    ${expectediptableentry}     ${ruletype}=ACCEPT    ${table}=INPUT
    @{ips} =    get_ips   ${url}   2
    FOR    ${ip}    IN    @{ips}
        Run Shell Process   iptables -A ${table} -s ${ip} -j ${ruletype}   OnError=Failed add ${ip} ${ruletype} rule to ${table} table
    END
    #Note this only checks one entry is there, possibly worth extending
    Check Iptable Rule Exists    ${expectediptableentry}


Check Iptable Rule Exists
    [Arguments]    ${iptableentry}=${TEST_IPADDR}
    ${result} =   Run Shell Process   iptables -L   OnError=Failed to list iptable rules
    Log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}
    Should Contain      ${result.stdout}    ${iptableentry}


Delete Iptable Rule
    [Arguments]    ${table}=INPUT
    Run Shell Process   iptables -F ${table}  OnError=Failed to flush ${table}


#Nftable
Add Nftable Rule From IP
    [Arguments]    ${ip}=${TEST_IPADDR}    ${table}=filter    ${chain}=OUTPUT
    Check Nftable Rule Can Be Set    ${table}   ${chain}
    ${result} =   Run Shell Process   nft add rule ip ${table} ${chain} ip daddr ${ip} counter drop   OnError=Failed add rule to nftables
    log  ${result.stdout}
    Should Be Equal As Integers  ${result.rc}  ${0}
    Check Nftable Rule Exists    ${ip}


Add Nftable Rule From URL
    [Arguments]    ${url}    ${table}=filter    ${chain}=OUTPUT
    @{ips} =    get_ips   ${url}   2
    Check Nftable Rule Can Be Set    ${table}   ${chain}
    FOR    ${ip}    IN    @{ips}
        Run Shell Process   nft add rule ip ${table} ${chain} ip daddr ${ip} counter accept   OnError=Failed add rule to nftables
        Check Nftable Rule Exists    ${ip}
    END

    [Return]    @{ips}


Check Nftable Multiple Rules Exist
    [Arguments]    ${ips}   ${table}=filter    ${chain}=OUTPUT
    FOR    ${ip}  IN  @{ips}
        Check Nftable Rule Exists    ${ip}     ${table}    ${chain}
    END

Check Nftable Rule Exists
    [Arguments]    ${ip}=${TEST_IPADDR}    ${table}=filter    ${chain}=OUTPUT
    ${result} =   Run Shell Process   nft list chain ip ${table} ${chain}  OnError=Failed to list nftable rules
    Log  ${result.stdout}
    Should Contain      ${result.stdout}    ${ip}


Flush Nftable Chain
    [Arguments]    ${table}=filter    ${chain}=OUTPUT
    Run Shell Process    nft flush chain ip ${table} ${chain}   OnError=Failed to Flush Nftable


Check Nftable Rule Can Be Set
    [Arguments]    ${table}    ${chain}
    Add Nft Table If It Doesnt Exist    ${table}
    Add Nft Chain If It Doesnt Exist     ${table}    ${chain}


Add Nft Table If It Doesnt Exist
    [Arguments]    ${table}
    ${status}  ${result} =   Run Keyword And Ignore Error     Run Shell Process   nft list tables ip ${table}  OnError=Failed to list nft table
    Log    ${result}
    IF    '${status}' == 'FAIL'
        Add Nft Table   ${table}
    END


Add Nft Table
    [Arguments]    ${table}
    Run Shell Process   nft add table ip ${table}    OnError=Failed to add nft table


Add Nft Chain If It Doesnt Exist
    [Arguments]    ${table}        ${chain}
    ${status}  ${result} =   Run Keyword And Ignore Error   Run Shell Process   nft list chain ip ${table} ${chain}  OnError=Failed to list nft chain
    Log    ${result}
    IF    '${status}' == 'FAIL'
        Add Nft Chain   ${table}    ${chain}
    END


Add Nft Chain
    [Arguments]    ${table}       ${chain}
    Run Shell Process   nft add chain ${table} ${chain}   OnError=Failed to add nft chain