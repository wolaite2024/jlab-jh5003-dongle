@echo off
set ic_type=%1
set target=%2

1>NUL copy .\objects\remote.lib    ..\..\..\..\bin\rtl87x%ic_type%\framework\

if "%target%" == "" (
	echo %target%
) else (
	1>NUL copy .\objects\remote.lib    ..\..\..\..\bin\rtl87x%ic_type%\framework\remote_%target%.lib
)
