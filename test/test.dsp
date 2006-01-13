# Microsoft Developer Studio Project File - Name="test" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=test - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "test.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "test.mak" CFG="test - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "test - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "test - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "test"
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "test - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /Ob2 /I "..\valib" /I "..\liba52" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "LIBA52_DOUBLE" /D "AC3_DEBUG" /D "AC3_DEBUG_NODITHER" /D "AC3_DEBUG_NOIMDCT" /D "TIME_WIN32" /FAs /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386

!ELSEIF  "$(CFG)" == "test - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /ZI /Od /I "..\valib" /I "..\liba52" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "LIBA52_DOUBLE" /D "AC3_DEBUG" /D "AC3_DEBUG_NODITHER" /D "AC3_DEBUG_NOIMDCT" /D "TIME_WIN32" /FAs /Fr /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "test - Win32 Release"
# Name "test - Win32 Debug"
# Begin Group "liba52"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\liba52\a52.h
# End Source File
# Begin Source File

SOURCE=..\liba52\a52_bitstream.c
# End Source File
# Begin Source File

SOURCE=..\liba52\a52_bitstream.h
# End Source File
# Begin Source File

SOURCE=..\liba52\a52_internal.h
# End Source File
# Begin Source File

SOURCE=..\liba52\a52_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\liba52\a52_parser.h
# End Source File
# Begin Source File

SOURCE=..\liba52\bit_allocate.c
# End Source File
# Begin Source File

SOURCE=..\liba52\config.h
# End Source File
# Begin Source File

SOURCE=..\liba52\downmix.c
# End Source File
# Begin Source File

SOURCE=..\liba52\imdct.c
# End Source File
# Begin Source File

SOURCE=..\liba52\inttypes.h
# End Source File
# Begin Source File

SOURCE=..\liba52\parse.c
# End Source File
# Begin Source File

SOURCE=..\liba52\tables.h
# End Source File
# End Group
# Begin Group "tests"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\test_crash.cpp
# End Source File
# Begin Source File

SOURCE=.\test_empty.cpp
# End Source File
# Begin Source File

SOURCE=.\test_filter.cpp
# End Source File
# Begin Source File

SOURCE=.\test_float.cpp
# End Source File
# Begin Source File

SOURCE=.\test_null.cpp
# End Source File
# Begin Source File

SOURCE=.\test_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\test_spdifer.cpp
# End Source File
# End Group
# Begin Group "valib"

# PROP Default_Filter ""
# Begin Group "filters"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\filters\agc.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\agc.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\bass_redir.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\bass_redir.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\convert.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\convert.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\decoder.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\dejitter.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\dejitter.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\delay.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\delay.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\demux.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\demux.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\dvd_decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\dvd_decoder.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\filter_chain.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\filter_chain.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\levels.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\levels.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\mixer.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\mixer.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\proc.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\proc.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\spdifer.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\spdifer.h
# End Source File
# End Group
# Begin Group "parsers"

# PROP Default_Filter ""
# Begin Group "ac3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_bitalloc.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_bitalloc.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_defs.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_dither.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_enc.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_enc.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_imdct.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_imdct.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_mdct.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_mdct.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_tables.h
# End Source File
# End Group
# Begin Group "mpa"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_defs.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_synth.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_synth.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_synth_filter.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_tables.h
# End Source File
# End Group
# Begin Group "dts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_defs.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_tables.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_tables_adpcm.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_tables_fir.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_tables_huffman.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_tables_quantization.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_tables_vq.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\valib\parsers\file_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\file_parser.h
# End Source File
# End Group
# Begin Group "sink"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\sink\sink_raw.h
# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\win32\cpu.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\win32\cpu.h
# End Source File
# Begin Source File

SOURCE=..\valib\win32\thread.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\win32\thread.h
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\auto_file.h
# End Source File
# Begin Source File

SOURCE=..\valib\bitstream.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\bitstream.h
# End Source File
# Begin Source File

SOURCE=..\valib\crc.h
# End Source File
# Begin Source File

SOURCE=..\valib\data.h
# End Source File
# Begin Source File

SOURCE=..\valib\defs.h
# End Source File
# Begin Source File

SOURCE=..\valib\filter.h
# End Source File
# Begin Source File

SOURCE=..\valib\log.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\log.h
# End Source File
# Begin Source File

SOURCE=..\valib\mpeg_demux.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\mpeg_demux.h
# End Source File
# Begin Source File

SOURCE=..\valib\parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\rng.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\rng.h
# End Source File
# Begin Source File

SOURCE=..\valib\sink.h
# End Source File
# Begin Source File

SOURCE=..\valib\spk.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\spk.h
# End Source File
# Begin Source File

SOURCE=..\valib\sync.h
# End Source File
# Begin Source File

SOURCE=..\valib\vtime.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\vtime.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\source\noise.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\source\noise.h
# End Source File
# Begin Source File

SOURCE=..\valib\source\raw_source.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\source\raw_source.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\common.cpp
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\test.cpp
# End Source File
# End Target
# End Project
