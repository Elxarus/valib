@echo off

cd ..\..\samples\test
..\..\valib\test\test.exe
move test.log ..\..\valib\test
cd ..\..\valib\test
