@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S %0 %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl
@rem ';
#!perl
#line 15
use strict;

system('_clear.bat');
unlink('release\test.exe')
  if (-e 'release\test.exe');

system('msdev test.dsp /MAKE "test - Win32 Release" /REBUILD')
  && die "Compile failed!!!";

system('_clear.bat');

system('release\test.exe') 
  && die "Test failed!!!!";

__END__
:endofperl
