@echo off

for %%r in (release x64\release debug x64\debug ) do if exist "%%r\test.exe" (
  cd ..\..\samples\test
  ..\..\valib\test\%%r\test.exe
  move test.log ..\..\valib\test\%%r
  cd ..\..\valib\test
)
