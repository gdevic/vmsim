# Microsoft Developer Studio Project File - Name="VMgui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=VMgui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VMgui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VMgui.mak" CFG="VMgui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VMgui - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "VMgui - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/", aaaaaaaa"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "VMgui - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "GUI" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:windows /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=xcopy /Y Release\VMgui.exe ..\Install	echo -------------------------------------	echo Finished building Release executable	echo -------------------------------------
# End Special Build Tool

!ELSEIF  "$(CFG)" == "VMgui - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /GX /ZI /Od /I "..\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "GUI" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=xcopy /Y Debug\VMgui.exe ..\Install	echo -----------------------------------	echo Finished building Debug executable	echo -----------------------------------
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "VMgui - Win32 Release"
# Name "VMgui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CmosAndRtc.cpp
# End Source File
# Begin Source File

SOURCE=.\DiskIO.cpp
# End Source File
# Begin Source File

SOURCE=.\Floppy.cpp
# End Source File
# Begin Source File

SOURCE=.\InstDrv.cpp
# End Source File
# Begin Source File

SOURCE=.\Interface.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Memory.cpp
# End Source File
# Begin Source File

SOURCE=.\Progress.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsMachine.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsVM.cpp
# End Source File
# Begin Source File

SOURCE=.\Simulator.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TabCmos.cpp
# End Source File
# Begin Source File

SOURCE=.\TabFloppy.cpp
# End Source File
# Begin Source File

SOURCE=.\TabIde.cpp
# End Source File
# Begin Source File

SOURCE=.\TabVmm.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadDevice.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadMessages.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadMgr.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadMonochrome.cpp
# End Source File
# Begin Source File

SOURCE=.\ThreadVGA.cpp
# End Source File
# Begin Source File

SOURCE=.\VMgui.cpp
# End Source File
# Begin Source File

SOURCE=.\VMgui.odl
# End Source File
# Begin Source File

SOURCE=.\VMgui.rc
# End Source File
# Begin Source File

SOURCE=.\VMguiDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\VMguiView.cpp
# End Source File
# Begin Source File

SOURCE=.\WndDbgMessages.cpp
# End Source File
# Begin Source File

SOURCE=.\WndMonochrome.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Functions.h
# End Source File
# Begin Source File

SOURCE=.\InstDrv.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Progress.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SettingsMachine.h
# End Source File
# Begin Source File

SOURCE=.\SettingsVM.h
# End Source File
# Begin Source File

SOURCE=.\Simulator.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TabCmos.h
# End Source File
# Begin Source File

SOURCE=.\TabFloppy.h
# End Source File
# Begin Source File

SOURCE=.\TabIde.h
# End Source File
# Begin Source File

SOURCE=.\TabVmm.h
# End Source File
# Begin Source File

SOURCE=.\ThreadDevice.h
# End Source File
# Begin Source File

SOURCE=.\ThreadMessages.h
# End Source File
# Begin Source File

SOURCE=.\ThreadMgr.h
# End Source File
# Begin Source File

SOURCE=.\ThreadMonochrome.h
# End Source File
# Begin Source File

SOURCE=.\ThreadVGA.h
# End Source File
# Begin Source File

SOURCE=.\UserCodes.h
# End Source File
# Begin Source File

SOURCE=.\VMgui.h
# End Source File
# Begin Source File

SOURCE=.\VMguiDoc.h
# End Source File
# Begin Source File

SOURCE=.\VMguiView.h
# End Source File
# Begin Source File

SOURCE=.\WndDbgMessages.h
# End Source File
# Begin Source File

SOURCE=.\WndMonochrome.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\CMOS.bin
# End Source File
# Begin Source File

SOURCE=.\res\Face1.ico
# End Source File
# Begin Source File

SOURCE=.\res\Face2.ico
# End Source File
# Begin Source File

SOURCE=.\res\SystemBIOS.bin
# End Source File
# Begin Source File

SOURCE=.\res\TestStub.bin
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\VgaROM.bin
# End Source File
# Begin Source File

SOURCE=.\res\VMgui.ico
# End Source File
# Begin Source File

SOURCE=.\res\VMgui.rc2
# End Source File
# Begin Source File

SOURCE=.\res\VMguiDoc.ico
# End Source File
# End Group
# Begin Group "Include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Include\Intel.h
# End Source File
# Begin Source File

SOURCE=..\Include\Interface.h
# End Source File
# Begin Source File

SOURCE=..\Include\Ioctl.h
# End Source File
# Begin Source File

SOURCE=..\Include\Vm.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\dos.dsk
# End Source File
# Begin Source File

SOURCE=.\VMgui.reg
# End Source File
# End Target
# End Project
