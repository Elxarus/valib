@echo off

cd ..\..\samples\test
..\..\valib\test\release\test.exe
move test.log ..\..\valib\test
cd ..\..\valib\test
