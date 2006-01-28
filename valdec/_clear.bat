@echo off
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

del perf\*.pdb 2> nul
del perf\*.bsc 2> nul
del perf\*.exp 2> nul
del perf\*.idb 2> nul
del perf\*.lib 2> nul
del perf\*.obj 2> nul
del perf\*.pch 2> nul
del perf\*.res 2> nul
del perf\*.sbr 2> nul
del perf\*.ilk 2> nul
del perf\*.asm 2> nul
del perf\*.map 2> nul
