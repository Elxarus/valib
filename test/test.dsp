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
# ADD CPP /nologo /G6 /MD /W3 /Zi /O2 /Oy- /Ob2 /I "..\valib" /I "..\liba52" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "LIBA52_DOUBLE" /D "AC3_DEBUG" /D "AC3_DEBUG_NODITHER" /D "AC3_DEBUG_NOIMDCT" /D "TIME_WIN32" /FAs /Fr /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /map /debug /debugtype:both /machine:I386
# SUBTRACT LINK32 /nodefaultlib

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
# ADD CPP /nologo /G6 /MDd /W3 /GX /ZI /Od /I "..\valib" /I "..\liba52" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "LIBA52_DOUBLE" /D "AC3_DEBUG" /D "AC3_DEBUG_NODITHER" /D "AC3_DEBUG_NOIMDCT" /D "TIME_WIN32" /FAs /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /machine:I386

!ENDIF 

# Begin Target

# Name "test - Win32 Release"
# Name "test - Win32 Debug"
# Begin Group "tests"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\test_ac3.cpp
# End Source File
# Begin Source File

SOURCE=.\test_bs_convert.cpp
# End Source File
# Begin Source File

SOURCE=.\test_crash.cpp
# End Source File
# Begin Source File

SOURCE=.\test_crc.cpp
# End Source File
# Begin Source File

SOURCE=.\test_decodergraph.cpp
# End Source File
# Begin Source File

SOURCE=.\test_demux.cpp
# End Source File
# Begin Source File

SOURCE=.\test_despdifer.cpp
# End Source File
# Begin Source File

SOURCE=.\test_detector.cpp
# End Source File
# Begin Source File

SOURCE=.\test_dvdgraph.cpp
# End Source File
# Begin Source File

SOURCE=.\test_filter.cpp
# End Source File
# Begin Source File

SOURCE=.\test_filtergraph.cpp
# End Source File
# Begin Source File

SOURCE=.\test_fir.cpp
# End Source File
# Begin Source File

SOURCE=.\test_general.cpp
# End Source File
# Begin Source File

SOURCE=.\test_null.cpp
# End Source File
# Begin Source File

SOURCE=.\test_parser_filter.cpp
# End Source File
# Begin Source File

SOURCE=.\test_proc.cpp
# End Source File
# Begin Source File

SOURCE=.\test_spdifer.cpp
# End Source File
# Begin Source File

SOURCE=.\test_streambuf.cpp
# End Source File
# Begin Source File

SOURCE=.\test_syncer.cpp
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

SOURCE=..\valib\filters\convolver.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\convolver.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\counter.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\decoder.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\decoder_graph.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\decoder_graph.h
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

SOURCE=..\valib\filters\detector.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\detector.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\dvd_graph.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\dvd_graph.h
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

SOURCE=..\valib\filters\parser_filter.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\parser_filter.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\proc.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\proc.h
# End Source File
# Begin Source File

SOURCE=..\valib\filters\resample.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filters\resample.h
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

SOURCE=..\valib\parsers\ac3\ac3_header.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\ac3\ac3_header.h
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

SOURCE=..\valib\parsers\mpa\mpa_header.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\mpa\mpa_header.h
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

SOURCE=..\valib\parsers\dts\dts_header.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\dts\dts_header.h
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
# Begin Group "spdif"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\parsers\spdif\spdif_header.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\spdif\spdif_header.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\spdif\spdif_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\spdif\spdif_parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\spdif\spdif_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\spdif\spdif_wrapper.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\valib\parsers\file_parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\file_parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\multi_frame.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\multi_frame.h
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\multi_header.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parsers\multi_header.h
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

SOURCE=..\valib\crc.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\crc.h
# End Source File
# Begin Source File

SOURCE=..\valib\data.h
# End Source File
# Begin Source File

SOURCE=..\valib\defs.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\defs.h
# End Source File
# Begin Source File

SOURCE=..\valib\divisors.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\divisors.h
# End Source File
# Begin Source File

SOURCE=..\valib\filter.h
# End Source File
# Begin Source File

SOURCE=..\valib\filter_graph.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\filter_graph.h
# End Source File
# Begin Source File

SOURCE=..\valib\filter_tester.h
# End Source File
# Begin Source File

SOURCE=..\valib\fir.h
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

SOURCE=..\valib\syncscan.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\syncscan.h
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
# Begin Group "dsp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\dsp\dbesi0.c
# End Source File
# Begin Source File

SOURCE=..\valib\dsp\dbesi0.h
# End Source File
# Begin Source File

SOURCE=..\valib\dsp\fftsg_ld.c
# End Source File
# Begin Source File

SOURCE=..\valib\dsp\fftsg_ld.h
# End Source File
# Begin Source File

SOURCE=..\valib\dsp\kaiser.h
# End Source File
# End Group
# Begin Group "fir"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\valib\fir\param_ir.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\fir\param_ir.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\all_filters.h
# End Source File
# Begin Source File

SOURCE=.\common.cpp
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\suite.h
# End Source File
# Begin Source File

SOURCE=.\test.cpp
# End Source File
# Begin Source File

SOURCE=.\test_source.h
# End Source File
# End Target
# End Project
