@echo off
set flash_bank_path=%1
set flash_bank_num=%2
set ic_type=%3

cmd.exe /c copy ..\..\..\..\bin\%ic_type%\flash_map_config\%flash_bank_path%\flash_%flash_bank_path%\flash_map.h ..\..\..\..\bin\%ic_type%\flash_map_config\flash_map.h

if "%flash_bank_path%" == "16M" (
    cmd.exe /c copy ..\..\..\..\bin\%ic_type%\upperstack_stamp\4M\upperstack_compile_stamp.h ..\..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h
) else (
    cmd.exe /c copy ..\..\..\..\bin\%ic_type%\upperstack_stamp\%flash_bank_path%\upperstack_compile_stamp.h ..\..\..\..\bin\%ic_type%\upperstack_stamp\upperstack_compile_stamp.h
)