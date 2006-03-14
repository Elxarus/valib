@echo off

set app=spdifer

call _clear.bat
del release\%app% 2> nul
msdev %app%.dsp /MAKE "%app% - Win32 Release" /REBUILD
call _clear.bat
