@echo off
set flash_bank_path=%1
set ic_type=%2
set target=%3
set target_2=%4
set sub_target=%5

copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
copy ..\..\..\bin\%ic_type%\upperstack_stamp\%flash_bank_path%\upperstack_compile_stamp.h ..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h
copy ..\..\..\bin\%ic_type%\gap_lib\gap_utils_%flash_bank_path%.lib ..\..\..\bin\%ic_type%\gap_utils.lib
copy ..\..\..\bin\%ic_type%\leaudio\leaudio.lib ..\..\..\bin\%ic_type%\leaudio.lib
copy ..\..\..\bin\%ic_type%\usb\usb.lib ..\..\..\bin\%ic_type%\usb.lib
copy ..\..\..\bin\%ic_type%\usb\usb_hal.lib ..\..\..\bin\%ic_type%\usb_hal.lib

if "%target%" == "" (
	echo %target%
) else (
	copy ..\..\..\bin\%ic_type%\framework\framework_%target%.lib ..\..\..\bin\%ic_type%\framework.lib
)

if "%target_2%" == "" (
	echo %target_2%
) else (
	copy ..\..\..\bin\%ic_type%\ble_mgr\ble_mgr_%target_2%.lib ..\..\..\bin\%ic_type%\ble_mgr.lib
)
if "%target%" == "stereo" (
	copy ..\..\..\bin\%ic_type%\ble_mgr\ble_mgr_all.lib ..\..\..\bin\%ic_type%\ble_mgr.lib
)

set is_original_upperstack_lib=F
if "%sub_target%" == "gaming_stereo" set is_original_upperstack_lib=T
if "%sub_target%" == "gaming_tws" set is_original_upperstack_lib=T
if "%is_original_upperstack_lib%" == "T" (
rem	copy ..\..\..\bin\%ic_type%\upperstack_4M_original.lib ..\..\..\bin\%ic_type%\upperstack_4M.lib
	copy ..\..\..\bin\%ic_type%\lib_gaming_common\4M\upperstack_4M.lib ..\..\..\bin\%ic_type%\upperstack_4M.lib
)

set is_flash_map_change=F
if "%sub_target%" == "gaming_stereo" set is_flash_map_change=T
if "%sub_target%" == "gaming_tws" set is_flash_map_change=T
if "%sub_target%" == "Voltron" set is_flash_map_change=T
if "%sub_target%" == "HoistGen2" set is_flash_map_change=T
if "%is_flash_map_change%" == "T" (
	copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_%sub_target%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
)

