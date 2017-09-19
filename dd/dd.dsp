# Microsoft Developer Studio Project File - Name="dd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=dd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dd.mak" CFG="dd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dd - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "dd - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/", aaaaaaaa"
# PROP Scc_LocalPath ".."
MTL=midl.exe

!IF  "$(CFG)" == "dd - Win32 Release"

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

!ELSEIF  "$(CFG)" == "dd - Win32 Debug"

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

!ENDIF 

# Begin Target

# Name "dd - Win32 Release"
# Name "dd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "c,cc"
# Begin Source File

SOURCE=.\CmosAndRtc.c
# End Source File
# Begin Source File

SOURCE=.\CPU.c
# End Source File
# Begin Source File

SOURCE=.\CPUTraps.c
# End Source File
# Begin Source File

SOURCE=.\Debug.c
# End Source File
# Begin Source File

SOURCE=.\Disassembler.c
# End Source File
# Begin Source File

SOURCE=.\DisassemblerForScanner.c
# End Source File
# Begin Source File

SOURCE=.\DisassemblerForVirtualization.c
# End Source File
# Begin Source File

SOURCE=.\Driver.c
# End Source File
# Begin Source File

SOURCE=.\ExternalInterrupts.c
# End Source File
# Begin Source File

SOURCE=.\FaultingInstructions.c
# End Source File
# Begin Source File

SOURCE=.\Initialization.c
# End Source File
# Begin Source File

SOURCE=.\Io.c
# End Source File
# Begin Source File

SOURCE=.\Keyboard.c
# End Source File
# Begin Source File

SOURCE=.\MainLoop.c
# End Source File
# Begin Source File

SOURCE=.\Memory.c
# End Source File
# Begin Source File

SOURCE=.\MemoryAllocation.c
# End Source File
# Begin Source File

SOURCE=.\Meta.c
# End Source File
# Begin Source File

SOURCE=.\MonochromeAdapter.c
# End Source File
# Begin Source File

SOURCE=.\Pci.c
# End Source File
# Begin Source File

SOURCE=.\Pic.c
# End Source File
# Begin Source File

SOURCE=.\Piix4.c
# End Source File
# Begin Source File

SOURCE=.\Pit.c
# End Source File
# Begin Source File

SOURCE=.\ProtectedInstructions.c
# End Source File
# Begin Source File

SOURCE=.\Queue.c
# End Source File
# Begin Source File

SOURCE=.\Scanner.c
# End Source File
# Begin Source File

SOURCE=.\TaskBridge.c
# End Source File
# Begin Source File

SOURCE=.\Vga.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h,hh"
# Begin Source File

SOURCE=.\Include\CPU.h
# End Source File
# Begin Source File

SOURCE=.\Include\Debug.h
# End Source File
# Begin Source File

SOURCE=.\Include\Device.h
# End Source File
# Begin Source File

SOURCE=.\Include\DeviceExtension.h
# End Source File
# Begin Source File

SOURCE=.\Include\DisassemblerData.h
# End Source File
# Begin Source File

SOURCE=.\Include\DisassemblerDefines.h
# End Source File
# Begin Source File

SOURCE=.\Include\DisassemblerInterface.h
# End Source File
# Begin Source File

SOURCE=.\Include\Globals.h
# End Source File
# Begin Source File

SOURCE=.\Include\Memory.h
# End Source File
# Begin Source File

SOURCE=.\Include\Monitor.h
# End Source File
# Begin Source File

SOURCE=.\Include\Queue.h
# End Source File
# Begin Source File

SOURCE=.\Include\Scanner.h
# End Source File
# End Group
# Begin Group "Other Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\makefile
# End Source File
# Begin Source File

SOURCE=.\Makefile.inc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# End Target
# End Project
