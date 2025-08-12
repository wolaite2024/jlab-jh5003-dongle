@echo off
setlocal enabledelayedexpansion

echo (*)Using the standard UAC prompt.

rem ***************************************************************
rem                  Request UAC prompt														
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

echo Generating time stamp...

rem set variables to this process/session only, not global
SETLOCAL ENABLEEXTENSIONS 

rem capture current time and remove colons
SET UNIQUETIME=%TIME::=%
rem take only first 6 characters of string
SET UNIQUETIME=%UNIQUETIME:~0,6%
rem create unique string through concatenation
SET UNIQUEPATHNAME=%COMPUTERNAME%%DATE:/=%_%UNIQUETIME%
rem strip out unnecessary characters
SET UNIQUEPATHNAME=%UNIQUEPATHNAME:.=_%
SET UNIQUEPATHNAME=%UNIQUEPATHNAME: =%

set LOGDIR="%PROGRAMDATA%\Realtek"
if not exist %LOGDIR% md %LOGDIR%

@echo enable realtek wpp log
set WPP_PATH="%LOGDIR%\MPPGTOOLTRACES"
if not exist %WPP_PATH% md %WPP_PATH%

rem Get the time from WMI - at least that's a format we can work with
set X=
for /f "skip=1 delims=" %%x in ('%systemroot%\System32\wbem\wmic.exe os get localdatetime') do if not defined X set X=%%x
echo.%X%

set DATE.YEAR=%X:~0,4%
set DATE.MONTH=%X:~4,2%
set DATE.DAY=%X:~6,2%
set DATE.HOUR=%X:~8,2%
set DATE.MINUTE=%X:~10,2%
set DATE.SECOND=%X:~12,2%

set CDATE=%DATE.YEAR%_%DATE.MONTH%_%DATE.DAY%
SET CTIME=%DATE.HOUR%-%DATE.MINUTE%-%DATE.SECOND%


if not exist %WPP_PATH%\%CDATE%_%CTIME% md %WPP_PATH%\%CDATE%_%CTIME%
set LOGDIR=%WPP_PATH%\%CDATE%_%CTIME%
set INFO_FILE="%LOGDIR%\SystemInfo%PROCESSOR_ARCHITECTURE%%CDATE%_%CTIME%"


echo ####################################################################
echo Log file is put in the Dir:
echo %LOGDIR%
echo ####################################################################

rem RtkMPPGExport
logman create trace -n rtkmppgexporttrace -o %LOGDIR%\RtkMPPGExport_%CTIME%.etl -nb 128 640 -bs 128
logman update trace -n rtkmppgexporttrace -p {E2B6B77D-25A2-4df7-83A2-4B15F5BA235E} 0xffffffff 5
logman start -n rtkmppgexporttrace

rem MPPGTool
logman create trace -n rtkmppgtooltrace -o %LOGDIR%\MPPGTool_%CTIME%.etl -nb 128 640 -bs 128
logman update trace -n rtkmppgtooltrace -p {7C95B9D7-E39D-4484-AC81-9695C570AC61} 0xffffffff 5
logman start -n rtkmppgtooltrace

:pause

@echo enable app dump
set APP_DUMP_PATH="%LOGDIR%\AppDump"
if not exist %APP_DUMP_PATH% md %APP_DUMP_PATH%

@REG ADD "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MPPGTool.exe" /v DumpCount /t  REG_DWORD /d 10 /f
@REG ADD "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MPPGTool.exe" /v DumpType /t  REG_DWORD /d 2 /f
@REG ADD "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MPPGTool.exe" /v CustomDumpFlags /t  REG_DWORD /d 0 /f
@REG ADD "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MPPGTool.exe" /v DumpFolder /t  REG_SZ /d %APP_DUMP_PATH% /f


echo ***************************************************************************
echo ***   Don't forget to run MPPGToolAutoLogDisable.cmd after finishing    ***
echo ***************************************************************************

:end
pause
