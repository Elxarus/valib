# Microsoft Developer Studio Project File - Name="valib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=valib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "valib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "valib.mak" CFG="valib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "valib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "valib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "valib - Win32 Perf" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "valib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /Ob2 /I "..\valib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "valib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\valib" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "valib - Win32 Perf"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Perf"
# PROP BASE Intermediate_Dir "Perf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Perf"
# PROP Intermediate_Dir "Perf"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /Ob2 /I "..\valib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "DOUBLE_SAMPLE" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /Ob2 /I "..\valib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "valib - Win32 Release"
# Name "valib - Win32 Debug"
# Name "valib - Win32 Perf"
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

SOURCE=..\valib\parser.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\parser.h
# End Source File
# Begin Source File

SOURCE=..\valib\pes_demux.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\pes_demux.h
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
# End Group
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

SOURCE=..\valib\sink\sink_dshow.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\sink\sink_dshow.h
# End Source File
# Begin Source File

SOURCE=..\valib\sink\sink_dsound.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\sink\sink_dsound.h
# End Source File
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
# Begin Source File

SOURCE=..\valib\win32\winspk.cpp
# End Source File
# Begin Source File

SOURCE=..\valib\win32\winspk.h
# End Source File
# End Group
# End Target
# End Project
