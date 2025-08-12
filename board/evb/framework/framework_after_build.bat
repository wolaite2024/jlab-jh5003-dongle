@echo off
rem 1>NUL del  ..\..\..\bin\framework.lib
set ic_type=%1
set target=%2

1>NUL copy .\objects\framework.lib    ..\..\..\bin\rtl87x%ic_type%\

if "%target%" == "" (
	echo %target%
) else (
	1>NUL copy .\objects\framework.lib    ..\..\..\bin\rtl87x%ic_type%\framework\framework_%target%.lib
)
