@echo off
if not "%1" == "Debug" (
	if not "%1" == "Release" (
		call :MakeError "PLease provide config build type: Debug | Release. For example: build.bat Debug"
                goto :EOF
	)
)
if not exist "libs" (
	call repo_init
)
if not exist "build" (
	call gen_prj
)
cmake --build build --config %1%
goto :EOF

:MakeError
if "%~1" == "" (
   goto :EOF
)
echo "%~1"
pause
goto :EOF