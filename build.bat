@echo off
if not "%1" == "Debug" (
	if not "%1" == "Release" (
		echo "PLease provide config build type: Debug | Release. For example: build.bat Debug"
		GOTO :EOF
	)
)
if not exist "build" (
	echo "Make sure you have run gen_prj first"
	GOTO :EOF
)
cmake --build build --config %1%