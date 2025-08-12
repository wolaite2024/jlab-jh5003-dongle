@echo off
set flash_bank_path=%1
set flash_bank_num=%2
set ic_type=%3
set target=%4
set sub_target=%5

::1>NUL copy ..\..\..\bin\bb_upperstack_bank%flash_bank_num%.lib 	..\..\..\bin\bb_upperstack.lib

cmd.exe /c copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h

if "%sub_target%"=="gaming_dongle" (
    echo "copy gaming dongle flash map"
    copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_%sub_target%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
)

cmd.exe /c copy ..\..\..\bin\%ic_type%\upperstack_stamp\%flash_bank_path%\upperstack_compile_stamp.h ..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h
copy ..\..\..\bin\%ic_type%\gap_lib\gap_utils_%flash_bank_path%.lib ..\..\..\bin\%ic_type%\gap_utils.lib
copy ..\..\..\bin\%ic_type%\framework\framework_all.lib ..\..\..\bin\%ic_type%\framework.lib
copy ..\..\..\bin\%ic_type%\gap_lib\gap_utils_%flash_bank_path%.lib ..\..\..\bin\%ic_type%\gap_utils.lib

if "%target%" == "" (
	echo %target%
) else (
	copy ..\..\..\bin\%ic_type%\leaudio\leaudio_%target%.lib ..\..\..\bin\%ic_type%\leaudio.lib
	copy ..\..\..\bin\%ic_type%\ble_mgr\ble_mgr_%target%.lib ..\..\..\bin\%ic_type%\ble_mgr.lib
)

copy ..\..\..\bin\%ic_type%\usb\usb.lib ..\..\..\bin\%ic_type%\usb.lib
copy ..\..\..\bin\%ic_type%\usb\usb_hal.lib ..\..\..\bin\%ic_type%\usb_hal.lib

set is_SS_Proj=F
if "%sub_target%" == "Voltron" set is_SS_Proj=T
if "%sub_target%" == "HoistGen2" set is_SS_Proj=T
if "%sub_target%" == "Voltron_X" set is_SS_Proj=T
if "%sub_target%" == "HoistGen2_X" set is_SS_Proj=T
if "%is_SS_Proj%" == "T" (
	echo "copy usb hal lib for SS"
    copy ..\..\..\bin\%ic_type%\usb\usb_hal_SS.lib ..\..\..\bin\%ic_type%\usb_hal.lib
	echo "copy usb lib for SS"	
	copy ..\..\..\bin\%ic_type%\usb\usb_SS.lib ..\..\..\bin\%ic_type%\usb.lib
)

if "%is_SS_Proj%" == "F" (
rem	copy ..\..\..\bin\%ic_type%\upperstack_4M_original.lib ..\..\..\bin\%ic_type%\upperstack_4M.lib
	copy ..\..\..\bin\%ic_type%\lib_gaming_common\4M\upperstack_4M.lib ..\..\..\bin\%ic_type%\upperstack_4M.lib
)

set is_SS_GIP_USB=F
if "%sub_target%" == "Voltron_X" set is_SS_GIP_USB=T
if "%sub_target%" == "HoistGen2_X" set is_SS_GIP_USB=T
if "%is_SS_GIP_USB%" == "T" (
	echo "copy usb GIP lib for SS"	
	copy ..\..\..\bin\%ic_type%\usb\usb_SS_GIP.lib ..\..\..\bin\%ic_type%\usb.lib
)

set is_Voltron_flash_map=F
if "%sub_target%" == "Voltron" set is_Voltron_flash_map=T
if "%sub_target%" == "Voltron_X" set is_Voltron_flash_map=T
if "%is_Voltron_flash_map%"=="T" (
    copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_Voltron\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
)

set is_HoistGen2_flash_map=F
if "%sub_target%" == "HoistGen2" set is_HoistGen2_flash_map=T
if "%sub_target%" == "HoistGen2_X" set is_HoistGen2_flash_map=T
if "%is_HoistGen2_flash_map%"=="T" (
    copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_HoistGen2\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
)

