@echo off

rem 1>NUL del  ..\..\..\bin\usb.lib
set ic_type=%1
set target=%2

1>NUL copy .\objects\usb.lib    ..\..\..\bin\rtl87x%ic_type%\usb.lib

if "%target%" == "" (
	echo %target%
	1>NUL copy .\objects\usb.lib    ..\..\..\bin\rtl87x%ic_type%\usb\usb.lib
) else (
	1>NUL copy .\objects\usb.lib    ..\..\..\bin\rtl87x%ic_type%\usb\usb_%target%.lib
)