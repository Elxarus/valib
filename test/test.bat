@echo off

set app=test

call _clear.bat
del release\%app% 2> nul
msdev %app%.dsp /MAKE "%app% - Win32 Release" /REBUILD
call _clear.bat

cd ..\..\samples\test
..\..\valib\test\release\test.exe
move test.log ..\..\valib\test
cd ..\..\valib\test
