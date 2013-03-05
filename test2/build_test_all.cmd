call build.cmd debug
call build.cmd release
call build.cmd x64 debug
call build.cmd x64 release

start /min cmd /c test.cmd debug --log_level=test_suite --log_sink=debug.log
start /min cmd /c test.cmd release --log_level=test_suite --log_sink=release.log
start /min cmd /c test.cmd x64\debug --log_level=test_suite --log_sink=debug64.log
start /min cmd /c test.cmd x64\release --log_level=test_suite --log_sink=release64.log
