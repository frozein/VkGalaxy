@echo off
Setlocal EnableDelayedExpansion

cd assets/shaders/

for /r %%i in (*) do (
	set input=%%i
	set output=!input:\assets\shaders\=\assets\spirv\!.spv

	set pathOfInput=%%~dpi
	set pathToCreate=!pathOfInput:\assets\shaders\=\assets\spirv\!
	mkdir !pathToCreate! 2>NUL

	echo Compiling shader %%i
	glslc !input! -o !output!
)

cd ../..