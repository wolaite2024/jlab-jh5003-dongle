echo off

set FROMELF_EXE=%1
set AXF_FILE=%2
set TARGET_NAME=%3
set TARGET_BANK=%4
set TARGET_IC=%5
set SUB_TARGET=%6
set TARGET_DIR=bin\%TARGET_IC%

set TARGET_MAP_PATH=Listings
set TARGET_OBJ_PATH=Objects

set FLASH_MAP_INI=..\..\..\%TARGET_DIR%\flash_map_config\4M\flash_4M\flash_map.ini
set FLASH_MAP_H=..\..\..\%TARGET_DIR%\flash_map_config\4M\flash_4M\flash_map.h
set HEX_IMAGE=app_image.hex
set MP_INI=mp.ini
set AES_KEY=..\..\..\tool\Gadgets\aes_key.bin
set RTK_RSA=..\..\..\tool\Gadgets\default_rsa_key.pem

set PREPEND_HEADER=..\..\..\tool\Gadgets\prepend_header.exe
set BUILD_DEBUG_TABLE=..\..\..\tool\Gadgets\build_debug_table_tool\build_debug_table.exe
set MD5=..\..\..\tool\Gadgets\md5_generate.sh
REM set MD5=..\..\..\tool\Gadgets\md5.exe
set BIN2HEX=..\..\..\tool\Gadgets\bin2hex.bat
set SREC_CAT=..\..\..\tool\Gadgets\srec_cat.exe

rd /s /q "%TARGET_DIR%\%TARGET_BANK%"
mkdir "%TARGET_DIR%\%TARGET_BANK%"

rem generate bin and disasm from axf
%FROMELF_EXE%  --bin -o "%TARGET_DIR%\%TARGET_BANK%" %AXF_FILE%
REM ---------------------------------------------
REM Optional: Generate a disassembly file for debugging.
REM To enable, remove the "::" at the beginning of the command.
REM Note: The "--interleave=source" flag interleaves source code with disassembled code.
REM This helps in correlating the source with its machine code, but the file will
REM contain your source code. Make sure to protect its confidentiality.
REM ---------------------------------------------
::%FROMELF_EXE%  -acd --interleave=source -o "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.disasm" %AXF_FILE%
copy "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%.bin" "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin"
copy "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%.trace" "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.trace"
del "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%.bin"
del "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%.trace"

rem generate MP bin for bank0

"%PREPEND_HEADER%"  /app_code "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin"
REM "%PREPEND_HEADER%" /aes_key "%AES_KEY%" /app_code "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin"
"%PREPEND_HEADER%" /rsa "%RTK_RSA%" /app_code "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin"

"%PREPEND_HEADER%" /mp_ini %MP_INI% /app_code "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin"
bash %MD5% "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%_MP.bin"
if exist "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%_MP.bin" del "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%_MP.bin"

copy "%TARGET_OBJ_PATH%\%TARGET_NAME%.htm" "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.htm"
copy "%TARGET_MAP_PATH%\%TARGET_NAME%.map" "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.map"

%BUILD_DEBUG_TABLE% --debug_dir "%TARGET_DIR%\%TARGET_BANK%" --out_dir "%TARGET_DIR%\%TARGET_BANK%"

set VERSION_FILE=../../../src/sample/%TARGET_NAME%/app_cfg.h

if "%SUB_TARGET%"=="gaming_dongle" (
	set VERSION_FILE=../../../src/sample/gaming_dongle/app_cfg.h
)

for /f "tokens=2,3" %%i in (%VERSION_FILE%) do (
    if %%i==RTL8763EAU_VER_MAJOR set TARGET_VER_MAJOR=%%j
    if %%i==RTL8763EAU_VER_MINOR set TARGET_VER_MINOR=%%j
	if %%i==RTL8763EAU_DBG_TITLE set TARGET_DBG_TITLE=%%j
)

:: Add special debug title in bin file name
if %TARGET_DBG_TITLE% == "" (
	set TARGET_VER=V%TARGET_VER_MAJOR%.%TARGET_VER_MINOR%
) else (
	set TARGET_VER=V%TARGET_VER_MAJOR%.%TARGET_VER_MINOR%-%TARGET_DBG_TITLE%
)


set TARGET_BIN_DIR=bin\\%TARGET_IC%\\%TARGET_BANK%\\
if not %TARGET_VER_MAJOR%=="" (
	for /f "delims=" %%i in ('dir /b "%TARGET_BIN_DIR%\\*%TARGET_NAME%*MP*.bin"') do (
		for /f "delims=- tokens=1,2,3,4" %%j in ("%%i") do ( move "%TARGET_BIN_DIR%\\%%i" "%TARGET_BIN_DIR%\\%%j-%TARGET_VER%-%%k-%%l-%%m" )
	)
)


REM generate hex for keil download
:: Get Bank0 & Bank1 App start address
for /f "tokens=2,3" %%i in (%FLASH_MAP_H%) do (
    if %%i==BANK0_APP_ADDR set BANK0_APP_ADDR_CFG=%%j
    if %%i==BANK1_APP_ADDR set BANK1_APP_ADDR_CFG=%%j
)

:: Select right app address for bank
if %TARGET_BANK% == bank1 (
    set APP_ADDR_CFG=%BANK1_APP_ADDR_CFG%
) else (
    set APP_ADDR_CFG=%BANK0_APP_ADDR_CFG%
)

:: Convert image from binary to Intel HEX format
%SREC_CAT% "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin" -binary -offset %APP_ADDR_CFG% -o "%TARGET_OBJ_PATH%\%HEX_IMAGE%" -intel

if exist "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%_MP*.bin" del "%TARGET_DIR%\%TARGET_BANK%\%TARGET_NAME%_%TARGET_BANK%.bin"

echo Output Image for Keil Download: "%TARGET_OBJ_PATH%\%HEX_IMAGE%"
echo Build Bank: %TARGET_BANK%
echo Download Address: %APP_ADDR_CFG%

echo on
