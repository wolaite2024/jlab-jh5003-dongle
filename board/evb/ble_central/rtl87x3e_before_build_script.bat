@echo off
set flash_bank_path=%1
set ic_type=%2
set target=%3
set sub_target=%4

copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
copy ..\..\..\bin\%ic_type%\leaudio\leaudio.lib ..\..\..\bin\%ic_type%\leaudio.lib

if "%flash_bank_path%" == "16M" (
    copy ..\..\..\bin\%ic_type%\upperstack_stamp\4M\upperstack_compile_stamp.h ..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h
	copy ..\..\..\bin\%ic_type%\gap_lib\gap_utils_4M.lib ..\..\..\bin\%ic_type%\gap_utils.lib
) else (
	copy ..\..\..\bin\%ic_type%\upperstack_stamp\%flash_bank_path%\upperstack_compile_stamp.h ..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h
	copy ..\..\..\bin\%ic_type%\gap_lib\gap_utils_%flash_bank_path%.lib ..\..\..\bin\%ic_type%\gap_utils.lib
)

if "%target%" == "" (
	echo %target%
) else (
	copy ..\..\..\bin\%ic_type%\framework\framework_%target%.lib ..\..\..\bin\%ic_type%\framework.lib
)

if "%target%" == "" (
	echo %target%
) else (
	copy ..\..\..\bin\%ic_type%\ble_mgr\ble_mgr_%target%.lib ..\..\..\bin\%ic_type%\ble_mgr.lib
)

if "%sub_target%" == "gaming" (
	copy ..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%_%sub_target%\flash_map.h ..\..\..\bin\%ic_type%\flash_map_config\flash_map.h
)
