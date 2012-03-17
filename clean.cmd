@echo off
call cmd\clean_all.cmd %*
rmdir /s /q doc\valib 2>nul
