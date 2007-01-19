@echo off

rem -------------------------------------------------------
rem msvc6
rem -------------------------------------------------------

del *.aps 2> nul
del *.ncb 2> nul
del *.plg 2> nul
del *.opt 2> nul
del *.dep 2> nul

rmdir /s /q debug 2> nul
del release\*.pdb 2> nul
del release\*.bsc 2> nul
del release\*.exp 2> nul
del release\*.idb 2> nul
del release\*.lib 2> nul
del release\*.obj 2> nul
del release\*.pch 2> nul
del release\*.res 2> nul
del release\*.sbr 2> nul
del release\*.ilk 2> nul
del release\*.asm 2> nul
del release\*.map 2> nul

rem -------------------------------------------------------
rem msvc8
rem -------------------------------------------------------

del /a:h *.suo 2> nul
del *.user 2> nul

rmdir /s /q debug_msvc8 2> nul
del release_msvc8\*.obj 2> nul
del release_msvc8\*.res 2> nul
del release_msvc8\*.sbr 2> nul
del release_msvc8\*.asm 2> nul
del release_msvc8\*.map 2> nul
del release_msvc8\*.pdb 2> nul
del release_msvc8\*.lib 2> nul
del release_msvc8\*.idb 2> nul
del release_msvc8\*.dep 2> nul
del release_msvc8\*.htm 2> nul
del release_msvc8\*.manifest 2> nul

rem -------------------------------------------------------
rem gcc
rem -------------------------------------------------------

del Makefile.win 2> nul
del test.layout 2> nul

rmdir /s /q gcc 2> nul


