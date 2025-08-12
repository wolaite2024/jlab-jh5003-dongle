@echo off

echo (*)Using the standard UAC prompt.

rem ***************************************************************
rem                  claim UAC prompt														
rem 
rem ***************************************************************

:: BatchGotAdmin
:-------------------------------------
REM  --> Check for permissions
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"

    wscript "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
    pushd "%CD%"
    CD /D "%~dp0"
:--------------------------------------

echo This script will turn off Realtek MPPGToolAutologger Tracing

setlocal enabledelayedexpansion

rem RtkMPPGExport
logman stop -n rtkmppgexporttrace 
logman delete -n rtkmppgexporttrace 

rem MPPGTool
logman stop -n rtkmppgtooltrace 
logman delete -n rtkmppgtooltrace 


@REG delete "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MPPGTool.exe" /f

echo Done!

echo Zip your C:\PragramData\Realtek folder and send to the person helping you 
echo Thanks!

:end
pause
