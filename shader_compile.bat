@echo off

setlocal EnableDelayedExpansion

for %%f in (assets/shaders/*) do (
	set "input=assets/shaders/%%~nxf"
	set "output=assets/spirv/%%~nf.spv"

    echo !input! --- !output!

    call "C:\Program Files\VulkanSDK\1.3.231.1\Bin\glslc.exe" !input! -o !output!

	echo.
)

pause