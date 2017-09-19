/******************************************************************************
*
*       NTRegmon.C - Windows NT Registry Monitor
*
*       Copyright (c) 1996 Mark Russinovich
*
*       You have the right to take this code and use it in whatever way you
*       wish.
*
*       PROGRAM: Instdrv.c
*
*       PURPOSE: Loads and unloads device drivers. This code
*       is taken from the instdrv example in the NT DDK.
*
******************************************************************************/
#include "stdafx.h"

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include <winioctl.h>
#include "Ioctl.h"
#include "vm.h"
#include "UserCodes.h"

#include <Winsvc.h>

int
InstallDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName,
    IN LPCTSTR    ServiceExe
    );

int
StartDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOL
OpenDevice(
    IN LPCTSTR    DriverName, HANDLE * lphDevice
    );

BOOL
StopDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOL
RemoveDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );



/****************************************************************************
*
*    FUNCTION: LoadDeviceDriver( const TCHAR, const TCHAR, HANDLE *)
*
*    PURPOSE: Registers a driver with the system configuration manager
*    and then loads it.
*
*    RETURNS: 0 on success
*             error code on failure
*
****************************************************************************/
int LoadDeviceDriver( const TCHAR * Name, const TCHAR * Path, HANDLE * lphDevice )
{
    OVERLAPPED  overlap = { 0 };
    DWORD       nb;
    SC_HANDLE   schSCManager;
    BOOL        okay;
    int         retval = 0;

    schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

    // Ignore success of installation: it may already be installed.
    if((retval=InstallDriver( schSCManager, Name, Path )) !=0)
    {
        // try to clean it up
        if (OpenDevice( Name, lphDevice ))
        {
            DeviceIoControl( *lphDevice, VMSIM_CleanItUp,
                NULL, 0,
                NULL, 0,
                &nb, &overlap );
            CloseHandle(*lphDevice);
            *lphDevice = INVALID_HANDLE_VALUE;
        }
        CloseServiceHandle( schSCManager );
        return retval;
    }

    if ((retval = StartDriver( schSCManager, Name )) != 0)      //error
    {
        CloseServiceHandle( schSCManager );
        return retval;
    }

    // Do make sure we can open it.
    okay = OpenDevice( Name, lphDevice );

    if(okay==FALSE)
    {
        retval = GetLastError();
    }

    CloseServiceHandle( schSCManager );

    return retval;
}


/****************************************************************************
*
*    FUNCTION: InstallDriver( IN SC_HANDLE, IN LPCTSTR, IN LPCTSTR)
*
*    PURPOSE: Creates a driver service.
*
*    RETURNS: 0 on success
*             error code on failure
*
****************************************************************************/
int InstallDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName, IN LPCTSTR ServiceExe )
{
    SC_HANDLE  schService;
    int retvalue = 0;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    schService = CreateService( SchSCManager,          // SCManager database
        DriverName,           // name of service
        DriverName,           // name to display
        SERVICE_ALL_ACCESS,    // desired access
        SERVICE_KERNEL_DRIVER, // service type
        SERVICE_DEMAND_START,  // start type
        SERVICE_ERROR_NORMAL,  // error control type
        ServiceExe,            // service's binary
        NULL,                  // no load ordering group
        NULL,                  // no tag identifier
        NULL,                  // no dependencies
        NULL,                  // LocalSystem account
        NULL                   // no password
        );
    if ( schService == NULL )
    {
        retvalue = GetLastError();
    }
    else
        CloseServiceHandle( schService );

    return retvalue;
}


/****************************************************************************
*
*    FUNCTION: StartDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Starts the driver service.
*
*    RETURNS: 0 on success
*             error code on failure
*
****************************************************************************/
int StartDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName )
{
    SC_HANDLE  schService;
    int ret = 0;

    schService = OpenService( SchSCManager,
        DriverName,
        SERVICE_ALL_ACCESS
        );
    if ( schService == NULL )
    {
        ret = GetLastError();
        return ret;
    }

    if((ret=StartService( schService, 0, NULL )) == 0)
    {
        ret = GetLastError();
        if( ret == ERROR_SERVICE_ALREADY_RUNNING )
            ret = 0;
    }
    else
        ret = 0;

    CloseServiceHandle( schService );

    return ret;
}



/****************************************************************************
*
*    FUNCTION: OpenDevice( IN LPCTSTR, HANDLE *)
*
*    PURPOSE: Opens the device and returns a handle if desired.
*
****************************************************************************/
BOOL OpenDevice( IN LPCTSTR DriverName, HANDLE * lphDevice )
{
    TCHAR    completeDeviceName[64];
    HANDLE   hDevice;

    //
    // Create a \\.\XXX device name that CreateFile can use
    //
    // NOTE: We're making an assumption here that the driver
    //       has created a symbolic link using it's own name
    //       (i.e. if the driver has the name "XXX" we assume
    //       that it used IoCreateSymbolicLink to create a
    //       symbolic link "\DosDevices\XXX". Usually, there
    //       is this understanding between related apps/drivers.
    //
    //       An application might also peruse the DEVICEMAP
    //       section of the registry, or use the QueryDosDevice
    //       API to enumerate the existing symbolic links in the
    //       system.
    //

    wsprintf( completeDeviceName, TEXT("\\\\.\\%s"), DriverName );

    hDevice = CreateFile( completeDeviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
        );
    int ret = GetLastError();
    if ( hDevice == ((HANDLE)-1) )
        return FALSE;

    // If user wants handle, give it to them.  Otherwise, just close it.
    if ( lphDevice )
        *lphDevice = hDevice;
    else
        CloseHandle( hDevice );

    return TRUE;
}


/****************************************************************************
*
*    FUNCTION: UnloadDeviceDriver( const TCHAR *, HANDLE lhDevice)
*
*    PURPOSE: Stops the driver and has the configuration manager unload it.
*
****************************************************************************/
BOOL UnloadDeviceDriver( const TCHAR * Name, HANDLE  lhDevice)
{
    SC_HANDLE   schSCManager;

    schSCManager = OpenSCManager(   NULL,                 // machine (NULL == local)
        NULL,                 // database (NULL == default)
        SC_MANAGER_ALL_ACCESS // access required
        );

    if (lhDevice != INVALID_HANDLE_VALUE)
        CloseHandle(lhDevice);
    StopDriver( schSCManager, Name );
    RemoveDriver( schSCManager, Name );
    //  RemoveDriver( schSCManager, Name );     // TODO: Do we really need
    //  RemoveDriver( schSCManager, Name );     // multiple calls of these?

    CloseServiceHandle( schSCManager );

    return TRUE;
}



/****************************************************************************
*
*    FUNCTION: StopDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Has the configuration manager stop the driver (unload it)
*
****************************************************************************/
BOOL StopDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName )
{
    SC_HANDLE       schService;
    BOOL            ret;
    SERVICE_STATUS  serviceStatus;

    schService = OpenService( SchSCManager, DriverName, SERVICE_ALL_ACCESS );
    if ( schService == NULL )
        return FALSE;

    ret = ControlService( schService, SERVICE_CONTROL_STOP, &serviceStatus );

    CloseServiceHandle( schService );

    return ret;
}


/****************************************************************************
*
*    FUNCTION: RemoveDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Deletes the driver service.
*
****************************************************************************/
BOOL RemoveDriver( IN SC_HANDLE SchSCManager, IN LPCTSTR DriverName )
{
    SC_HANDLE  schService;
    BOOL       ret;

    schService = OpenService( SchSCManager,
        DriverName,
        SERVICE_ALL_ACCESS
        );

    if ( schService == NULL )
        return FALSE;

    ret = DeleteService( schService );
    SERVICE_STATUS  serviceStatus;
    ret = QueryServiceStatus( schService, &serviceStatus);

    CloseServiceHandle( schService );

    return ret;
}

