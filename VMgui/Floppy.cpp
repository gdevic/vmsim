/******************************************************************************
*                                                                             *
*   Module:     Floppy.cpp                                                    *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       6/26/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the floppy disk support
        under NT.

        This file is based on mfmt.c Microsoft sample.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 6/26/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include "stdafx.h"
#include "VMgui.h"

#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"

#include "Simulator.h"

/******************************************************************************
*                                                                             *
*   Global Variables                                                          *
*                                                                             *
******************************************************************************/


/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/
DISK_GEOMETRY SupportedGeometry[20];
DWORD SupportedGeometryCount;

BOOL
GetDiskGeometry(
    HANDLE hDisk,
    PDISK_GEOMETRY lpGeometry
    )

{
    DWORD ReturnedByteCount;

    return DeviceIoControl( hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL, 0,
        lpGeometry, sizeof(*lpGeometry),
        &ReturnedByteCount, NULL );
}

DWORD
GetSupportedGeometrys(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;
    BOOL b;
    DWORD NumberSupported;

    b = DeviceIoControl( hDisk, IOCTL_DISK_GET_MEDIA_TYPES,
        NULL, 0,
        SupportedGeometry, sizeof(SupportedGeometry),
        &ReturnedByteCount, NULL );
    if ( b ) {
        NumberSupported = ReturnedByteCount / sizeof(DISK_GEOMETRY);
    }
    else {
        NumberSupported = 0;
    }
    SupportedGeometryCount = NumberSupported;

    return NumberSupported;
}

LPSTR GetGeometry( PDISK_GEOMETRY lpGeometry )
{
    LPSTR MediaType;

    switch ( lpGeometry->MediaType ) {
    case F5_1Pt2_512:  MediaType = "5.25, 1.2MB,  512 bytes/sector";break;
    case F3_1Pt44_512: MediaType = "3.5,  1.44MB, 512 bytes/sector";break;
    case F3_2Pt88_512: MediaType = "3.5,  2.88MB, 512 bytes/sector";break;
    case F3_20Pt8_512: MediaType = "3.5,  20.8MB, 512 bytes/sector";break;
    case F3_720_512:   MediaType = "3.5,  720KB,  512 bytes/sector";break;
    case F5_360_512:   MediaType = "5.25, 360KB,  512 bytes/sector";break;
    case F5_320_512:   MediaType = "5.25, 320KB,  512 bytes/sector";break;
    case F5_320_1024:  MediaType = "5.25, 320KB,  1024 bytes/sector";break;
    case F5_180_512:   MediaType = "5.25, 180KB,  512 bytes/sector";break;
    case F5_160_512:   MediaType = "5.25, 160KB,  512 bytes/sector";break;
    case RemovableMedia: MediaType = "Removable media other than floppy";break;
    case FixedMedia:   MediaType = "Fixed hard disk media";break;
    default:           MediaType = "Unknown";break;
    }

    return( MediaType );
    /*
    printf("    Media Type %s\n",MediaType);
    printf("    Cylinders %d Tracks/Cylinder %d Sectors/Track %d\n",
    lpGeometry->Cylinders.LowPart,
    lpGeometry->TracksPerCylinder,
    lpGeometry->SectorsPerTrack
    );
    */
}

BOOL
LowLevelFormat(
    HANDLE hDisk,
    PDISK_GEOMETRY lpGeometry
    )
{
    FORMAT_PARAMETERS FormatParameters;
    PBAD_TRACK_NUMBER lpBadTrack;
    UINT i;
    BOOL b;
    DWORD ReturnedByteCount;

    FormatParameters.MediaType = lpGeometry->MediaType;
    FormatParameters.StartHeadNumber = 0;
    FormatParameters.EndHeadNumber = lpGeometry->TracksPerCylinder - 1;
    lpBadTrack = (PBAD_TRACK_NUMBER) LocalAlloc(LMEM_ZEROINIT,lpGeometry->TracksPerCylinder*sizeof(*lpBadTrack));

    for (i = 0; i < lpGeometry->Cylinders.LowPart; i++) {

        FormatParameters.StartCylinderNumber = i;
        FormatParameters.EndCylinderNumber = i;

        b = DeviceIoControl( hDisk, IOCTL_DISK_FORMAT_TRACKS,
            &FormatParameters, sizeof(FormatParameters),
            lpBadTrack, lpGeometry->TracksPerCylinder*sizeof(*lpBadTrack),
            &ReturnedByteCount, NULL );

        if (!b ) {
            LocalFree(lpBadTrack);
            return b;
        }
    }

    LocalFree(lpBadTrack);

    return TRUE;
}

BOOL
LockVolume(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;

    return DeviceIoControl( hDisk, FSCTL_LOCK_VOLUME,
        NULL, 0,
        NULL, 0,
        &ReturnedByteCount, NULL );
}

BOOL
UnlockVolume(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;

    return DeviceIoControl( hDisk, FSCTL_UNLOCK_VOLUME,
        NULL, 0,
        NULL, 0,
        &ReturnedByteCount, NULL );
}

BOOL
DismountVolume(
    HANDLE hDisk
    )
{
    DWORD ReturnedByteCount;

    return DeviceIoControl( hDisk, FSCTL_DISMOUNT_VOLUME,
        NULL, 0,
        NULL, 0,
        &ReturnedByteCount, NULL );
}


HANDLE CSimulator::FloppyLock()
{
    char Drive[MAX_PATH];
    LPSTR DriveName = "A:";
    HANDLE hDrive;
    DISK_GEOMETRY Geometry;

    sprintf(Drive,"\\\\.\\%s",DriveName);
    hDrive = CreateFile( Drive, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        0, NULL );
    if ( hDrive == INVALID_HANDLE_VALUE )
    {
        ASSERT(0);
        //        printf("MFMT: Open %s failed %d\n",DriveName,GetLastError());
        return( 0 );
    }

    if ( LockVolume(hDrive) == FALSE )
    {
        ASSERT(0);
        //        printf("MFMT:Locking volume %s failed %d\n", DriveName, GetLastError());
        return( 0 );
    }

    if ( !GetDiskGeometry(hDrive,&Geometry) )
    {
        //        printf("MFMT: GetDiskGeometry %s failed %d\n",DriveName,GetLastError());
        return( 0 );
    }

    GetGeometry(&Geometry);

    return( hDrive );
}
