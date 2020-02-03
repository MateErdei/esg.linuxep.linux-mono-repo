*** Settings ***
Documentation    Tests for the installer
Library    OperatingSystem

Suite Setup  Remove Directory   ./tmp  recursive=True


*** Keywords ***
