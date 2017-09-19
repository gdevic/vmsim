/******************************************************************************
*                                                                             *
*   Module:     CmosAndRtc.c                                                  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/3/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the CMOS RAM and RTC virtual device.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/3/2000   Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

#include "DeviceExtension.h"        // Include our device extensions

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

#define BCD(x)  ((((x)/10)<<4) | ((x)%10))

static DWORD index;

// This table contains the values to be programmed with KeSetTimer()
// for the right time delay of the periodic interrupt

static LARGE_INTEGER PeriodicTimeTable[16] =
{
           0,           //   0000 None
        -390,           //   0001 3.90625  us
        -781,           //   0010 7.8125   us
       -1221,           //   0011 122.070  us
       -2441,           //   0100 244.141  us
       -4883,           //   0101 488.281  us
       -9766,           //   0110 976.562  us
      -19531,           //   0111 1.953125 ms
      -39063,           //   1000 3.90625  ms
      -78125,           //   1001 7.8125   ms
     -156250,           //   1010 15.625   ms
     -312500,           //   1011 31.25    ms
     -625000,           //   1100 62.5     ms
    -1250000,           //   1101 125      ms
    -2500000,           //   1110 250      ms
    -5000000            //   1111 500      ms
};

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int CmosAndRtc_Initialize(void);
static int CmosAndRtc_Start(void);
static int CmosAndRtc_Suspend(void);
static int CmosAndRtc_Resume(void);
static int CmosAndRtc_Shutdown(void);

static DWORD Cmos_fnIN(DWORD port, DWORD io_len);
static void Cmos_fnOUT(DWORD port, DWORD data, DWORD io_len);

static VOID DpcRtcA( IN PKDPC,IN PVOID,IN PVOID,IN PVOID );
static VOID DpcRtcB( IN PKDPC,IN PVOID,IN PVOID,IN PVOID );


/******************************************************************************
*                                                                             *
*   void CmosAndRtc_Register(TVDD *pVdd)                                      *
*                                                                             *
*******************************************************************************
*
*   Registration function for the CMOS and RTC subsystem.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void CmosAndRtc_Register(TVDD *pVdd)
{
    // This device operates timers, so we need to register all handlers
    // to start and stop timers as VM is being paused

    pVdd->Name       = "CMOS and RTC";
    pVdd->Initialize = CmosAndRtc_Initialize;
    pVdd->Start      = CmosAndRtc_Start;
    pVdd->Suspend    = CmosAndRtc_Suspend;
    pVdd->Resume     = CmosAndRtc_Resume;
    pVdd->Shutdown   = CmosAndRtc_Shutdown;
}


/******************************************************************************
*                                                                             *
*   int CmosAndRtc_Initialize()                                               *
*                                                                             *
*******************************************************************************
*
*   Initialization function for the virtual CMOS RAM and RTC device on the
*   port 0x70, 0x71
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int CmosAndRtc_Initialize()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    TRACE(("CmosAndRtc_Initialize()"));

    // Initialize status registers of the Rtc/CMOS to some constant values

    pVM->CMOS[0xA] = 0x26;      // Supported base and rate
    pVM->CMOS[0xB] = 0x02;      // 24-hour clock
    pVM->CMOS[0xC] = 0x00;      // Cleared
    pVM->CMOS[0xD] = 0x80;      // Battery operational

    RegisterIOPort(0x70, 0x71, Cmos_fnIN, Cmos_fnOUT );

    index = 0;

    pDevExt->PeriodicTimeA.QuadPart = -10000000;    // 10000000 100 ns (1 s)

    // Initialize DPC and the recurring timer for RTC timers

    KeInitializeDpc( &pDevExt->VmRtcDpcA, DpcRtcA, pDevExt );
    KeInitializeDpc( &pDevExt->VmRtcDpcB, DpcRtcB, pDevExt );

    KeInitializeTimer( &pDevExt->VmRtcTimerA );
    KeInitializeTimer( &pDevExt->VmRtcTimerB );

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int CmosAndRtc_Start()                                                    *
*                                                                             *
*******************************************************************************
*
*   Start the device. Start timers.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int CmosAndRtc_Start()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    // Start the 1Hz timer ticking...

    KeInsertQueueDpc(&pDevExt->VmRtcDpcA, NULL, NULL);

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int CmosAndRtc_Shutdown()                                                 *
*                                                                             *
*******************************************************************************
*
*   Shuts down device.  Deallocates timers etc.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int CmosAndRtc_Shutdown()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    // Destroy all the timers

    KeCancelTimer( &pDevExt->VmRtcTimerA );
    KeCancelTimer( &pDevExt->VmRtcTimerB );

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int CmosAndRtc_Suspend()                                                  *
*                                                                             *
*******************************************************************************
*
*   Temporary suspends execution of device.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int CmosAndRtc_Suspend()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    // Stop both timers

    KeCancelTimer( &pDevExt->VmRtcTimerA );
    KeCancelTimer( &pDevExt->VmRtcTimerB );

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int CmosAndRtc_Resume()                                                   *
*                                                                             *
*******************************************************************************
*
*   Resumes execution after the suspend message.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int CmosAndRtc_Resume()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    // Start the timer A (1Hz) ticking again, bu the timer B is started
    // only if enabled by the CMOS

    KeSetTimer( &pDevExt->VmRtcTimerA, pDevExt->PeriodicTimeA, &pDevExt->VmRtcDpcA );

    if( (pVM->CMOS[0xB] & 0x40) && (pDevExt->PeriodicTimeB.QuadPart != 0) )
    {
        // Periodic timer is enabled, so restart it

        KeSetTimer( &pDevExt->VmRtcTimerB, pDevExt->PeriodicTimeB, &pDevExt->VmRtcDpcB );
    }

    return( 0 );
}


//*****************************************************************************
//
//      TIMER CALLBACK FUNCTION: 1Hz
//
//*****************************************************************************
VOID DpcRtcA( IN PKDPC    Dpc,
              IN PVOID    DeferredContext,
              IN PVOID    SystemArgument1,
              IN PVOID    SystemArgument2 )
{
    PDEVICE_EXTENSION  pDevExt = DeferredContext;
    LARGE_INTEGER      DueTime;
    LARGE_INTEGER      SystemTime;
    TIME_FIELDS        TimeFields;
    BOOL fAlarm, fGenInt;

    if( pVM )
    {
        fGenInt = FALSE;

        // Dont update timer if the bit STAT_B[7] is 1

        if( !(pVM->CMOS[0xB] & 0x80) )
        {
            // Set the clock right (instead of advancing a second)
            // TODO: Speed optimization: explicitly advance a second

            KeQuerySystemTime( &SystemTime );
            RtlTimeToTimeFields( &SystemTime, &TimeFields );

            pVM->CMOS[0x00] = BCD(TimeFields.Second);
            pVM->CMOS[0x02] = BCD(TimeFields.Minute);
            pVM->CMOS[0x04] = BCD(TimeFields.Hour);

#if DBG 
        {
            // Test the timer stuff

            char str[80];
            sprintf(str, "UTC Time: %02X:%02X:%02X   CS:EIP=%02X:%04X",
                pVM->CMOS[0x04],pVM->CMOS[0x02],pVM->CMOS[0x00],
                pMonitor->guestTSS.cs, pMonitor->guestTSS.eip );
            OutputMessageToGUI(pDevExt, str);
        }
#endif
            // If update interrupts are enabled, trip IRQ 8

            if( pVM->CMOS[0xB] & 0x10 )     // Update interrupt
            {
                // IRQ request, source is update interrupt

                pVM->CMOS[0xC] = 0x80 | 0x10;
                fGenInt = TRUE;
            }

            // Compare current time/date to alarm time/date

            if( pVM->CMOS[0xB] & 0x20 )     // Alarm interrupt
            {
                fAlarm = TRUE;

                if( (pVM->CMOS[0x01] & 0xC0) != 0xC0 ) {
                    if( pVM->CMOS[0x00] != pVM->CMOS[0x01] )
                        fAlarm = FALSE;
                }
                if( (pVM->CMOS[0x03] & 0xC0) != 0xC0 ) {
                    if( pVM->CMOS[0x02] != pVM->CMOS[0x03] )
                        fAlarm = FALSE;
                }
                if( (pVM->CMOS[0x05] & 0xC0) != 0xC0 ) {
                    if( pVM->CMOS[0x04] != pVM->CMOS[0x05] )
                        fAlarm = FALSE;
                }

                if( fAlarm )
                {
                    // IRQ request, source is alarm interrupt

                    pVM->CMOS[0xC] |= 0x80 | 0x20;
                    fGenInt = TRUE;
                }
            }
        }

        // Generate interrupt if update or alarm happened

        if( fGenInt==TRUE )
            VmSIMGenerateInterrupt(8);
    }

    // Reset the update timer to happen again in a second

    if (pDevExt->runState == vmStateRunning)
    {
        KeSetTimer( &pDevExt->VmRtcTimerA, pDevExt->PeriodicTimeA, &pDevExt->VmRtcDpcA );
    }
}


//*****************************************************************************
//
//      PERIODIC TIMER CALLBACK FUNCTION
//
//*****************************************************************************
VOID DpcRtcB( IN PKDPC    Dpc,
              IN PVOID    DeferredContext,
              IN PVOID    SystemArgument1,
              IN PVOID    SystemArgument2 )
{
    PDEVICE_EXTENSION  pDevExt = DeferredContext;

    if( pVM )
    {
#if DBG
        {
            char str[80];
            sprintf(str, "RTC: Periodic timer activated");
            OutputMessageToGUI(pDevExt, str);
        }
#endif

        // If periodic interrupts are enabled, trip IRQ 8

        if( pVM->CMOS[0xB] & 0x40 )     // Periodic interrupt
        {
            // IRQ request, source is periodic timer

            pVM->CMOS[0xC] = 0x80 | 0x40;
            VmSIMGenerateInterrupt(8);
        }
    }

    // Reset the periodic timer to happen again

    if (pDevExt->runState == vmStateRunning)
    {
        KeSetTimer( &pDevExt->VmRtcTimerB, pDevExt->PeriodicTimeB, &pDevExt->VmRtcDpcB );
    }
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************
static DWORD Cmos_fnIN(DWORD port, DWORD io_len)
{
    BYTE value;
    TRACE(("Cmos_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));

    if( io_len==1 )
    {
        if( port == 0x71 )
        {
            value = pVM->CMOS[index];

            // All bits of register C are cleared after a read

            if( index==0x0C )
                pVM->CMOS[0x0C] = 0;

            return( value );
        }
        else
            WARNING(("Reading of a Write Only port 0x70"));
    }
    else
        WARNING(("Invalid io_len"));

    return( 0xFF );
}

static void Cmos_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    TRACE(("Cmos_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));
    ASSERT(io_len==1);

    if( io_len==1 )
    {
        if( port==0x70 )
        {
            // Address port

            index = data & 0x7F;
        }
        else
        {
            // Data port

            switch( index )
            {
                case 0xA:       // Control register A
                        data &= 0x7F;                   // Bit 7 is read-only
                        if( ((data >> 4)!=2) && ((data >> 4)!=0) )
                            ERROR(("Unsupported timer base"));
                        if( (data & 0x7)!=6 )
                            ERROR(("Unsupported periodic timer rate"));
                    break;

                case 0xB:       // Control register B
                        if( data & 4 )
                            ERROR(("Can not handle binary format"));
                        if( !(data & 2) )
                            ERROR(("Can not handle 12-hour mode"));
                        data &= 0xF7;                   // bit 3 is always 0
                        if( data & 0x80 )
                            data &= 0xEF;               // Setting bit 7 clears bit 4

                        if( (pVM->CMOS[0xB] & 0x40)==0 )
                        {
                            if( (data & 0x40)==0x40 )
                            {
                                // Periodic timer is being activated

                                pDevExt->PeriodicTimeB = PeriodicTimeTable[pVM->CMOS[0xA] & 0xF];

                                if( pDevExt->PeriodicTimeB.QuadPart != 0 )
                                    KeSetTimer( &pDevExt->VmRtcTimerB, pDevExt->PeriodicTimeB, &pDevExt->VmRtcDpcB );
                                else
                                    KeCancelTimer( &pDevExt->VmRtcTimerB );
                            }
                        }
                        else
                        {
                            if( (data & 0x40)==0 )
                            {
                                // Periodic timer is being deactivated

                                KeCancelTimer( &pDevExt->VmRtcTimerB );
                            }
                        }
                    break;

                case 0xC:                               // Invalid register to write
                case 0xD:                               // Invalid register to write
                        ERROR(("Write to a read only control register"));
                        data = pVM->CMOS[index];        // so revert to old value
                    break;

                case 0xF:
                        switch( data )
                        {
                            case 0: INFO(("CMOS: Shutdown - normal POST")); break;
                            case 2: INFO(("CMOS: Shutdown - after memory test")); break;
                            case 3: INFO(("CMOS: Shutdown - after memory test")); break;
                            case 4: INFO(("CMOS: Shutdown - jump to disk bootstrap")); break;
                            case 6: INFO(("CMOS: Shutdown - after memory test")); break;
                            case 9: INFO(("CMOS: Shutdown - BIOS ext memory move")); break;
                            case 10:INFO(("CMOS: Shutdown - jump to DWORD 40:67")); break;
                            default:INFO(("CMOS: Shutdown - Unsupported code")); break;
                        }
                    break;
            }

            pVM->CMOS[index] = (BYTE) data;
        }
    }
    else
        WARNING(("Invalid io_len"));
}

