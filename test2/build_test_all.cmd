call build.cmd debug
call build.cmd release
call build.cmd x64 debug
call build.cmd x64 release

call test.cmd debug --log_level=test_suite --log_sink=debug.log
call test.cmd release --log_level=test_suite --log_sink=release.log
call test.cmd x64\debug --log_level=test_suite --log_sink=debug64.log
call test.cmd x64\release --log_level=test_suite --log_sink=release64.log
