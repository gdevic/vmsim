/******************************************************************************
*                                                                             *
*   Module:     Pit.c                                                         *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/8/2000                                                      *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the PIT 8253/8254

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/8/2000   Original                                             Goran Devic *
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

typedef struct
{
    BOOL bcd;                           // counting in bcd
    DWORD mode;                         // mode of operation
    DWORD rw;                           // read/write modes

    WORD input_latch;                   // input latch value
    BOOL input_latch_toggle;

    WORD counter_max;                   // initial counter value (from latch)
    WORD counter;                       // current counter value
    LARGE_INTEGER DueTime;              // timer value for dpc callback

} TPit;                                 // counting element

TPit pit[3];

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Pit_Initialize(void);
static int Pit_Shutdown(void);

static DWORD Pit_fnIN(DWORD port, DWORD io_len);
static void Pit_fnOUT(DWORD port, DWORD data, DWORD io_len);
VOID DpcPit(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID A1, IN PVOID A2 );

/******************************************************************************
*                                                                             *
*   void Pit_Register(TVDD *pVdd)                                             *
*                                                                             *
*******************************************************************************
*
*   Registration function for the Programmable Interrupt Timer chip.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Pit_Register(TVDD *pVdd)
{
    pVdd->Name = "Programmable Interval Timer";
    pVdd->Initialize = Pit_Initialize;
    pVdd->Shutdown   = Pit_Shutdown;
}


/******************************************************************************
*                                                                             *
*   int Pit_Initialize()                                                      *
*                                                                             *
*******************************************************************************
*
*   Initialized virtual Programmable Interrupt Timer chip.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Pit_Initialize()
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    TRACE(("Pit_Initialize()"));

    memset(pit, 0, sizeof(pit));

    pit[0].mode = 2;            // Rate generator for IRQ 0
    pit[1].mode = 3;            // Square-Wave generator for DMA 0   (ignored)
    pit[0].mode = 2;            // Rate generator for speaker output (ignored)

    RegisterIOPort(0x40, 0x43, Pit_fnIN, Pit_fnOUT );

    // Initialize DPC and the recurring timer for the PIT counter 0

    KeInitializeDpc( &pDevExt->VmPitDpc0, DpcPit, pDevExt );

    KeInitializeTimer( &pDevExt->VmPitTimer0 );

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int Pit_Shutdown(void)                                                    *
*                                                                             *
*******************************************************************************
*
*   Shuts down device.  Deallocates timer.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Pit_Shutdown(void)
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;

    // Destroy all the timers

    KeCancelTimer( &pDevExt->VmPitTimer0 );

    return( 0 );
}


//*****************************************************************************
//
//      PIT COUNTER 0 CALLBACK FUNCTION
//
//*****************************************************************************
VOID DpcPit( IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID A1, IN PVOID A2 )
{
    PDEVICE_EXTENSION  pDevExt = DeferredContext;

    TRACE(("IRQ 0"));

    VmSIMGenerateInterrupt(0);

    // Reset the timer to continue

    if (pDevExt->runState != vmStateSuspended)
    {
        KeSetTimer( &pDevExt->VmPitTimer0, pit[0].DueTime, &pDevExt->VmPitDpc0 );
    }
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Pit_fnIN(DWORD address, DWORD io_len)
{
    TPit *p;

    TRACE(("Pit_fnIN(DWORD port=%04X, DWORD io_len=%X)", address, io_len));
    ASSERT(io_len==1);

    if( address==0x43 )
    {
        WARNING(("Reading write-only register 0x43"));
        return( 0 );
    }

    ASSERT(0);  // Not yet implemented

    return( 0 );
}

static void Pit_fnOUT(DWORD address, DWORD value, DWORD io_len)
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)GUIDevice->DeviceExtension;
    TPit *p;
    BOOL xfer_complete = 1;
    DWORD select;

    TRACE(("Pit_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", address, value, io_len));
    ASSERT(io_len==1);

    if( address==0x43 )
    {
        // Write control register

        select = value >> 6;
        ASSERT((select==0) || (select==1) || (select==2));

        pit[select].bcd  = value & 1;
        pit[select].mode = (value >> 1) & 7;
        pit[select].rw   = (value >> 4) & 3;
    }
    else
    {
        // Write one of the counters

        p = &pit[ address - 0x40 ];

        switch( p->rw )
        {
            case 0:             // Counter latch instruction
                    ASSERT(0);
                    xfer_complete = 0;
                break;

            case 1:             // Low counter byte only
                    p->input_latch = (WORD) value;
                break;

            case 2:             // High counter byte only
                    p->input_latch = ((WORD)value << 8);
                break;

            case 3:             // Low first, then High counter byte
                if( p->input_latch_toggle==0 )
                {
                    p->input_latch = (WORD) value;
                    p->input_latch_toggle = 1;
                    xfer_complete = 0;
                }
                else
                {
                    p->input_latch |= ((WORD) value << 8);
                    p->input_latch_toggle = 0;
                }
        }

        if( xfer_complete )
        {
            p->counter_max = p->input_latch;

            switch( p->mode )
            {
                case 2:                     // Rate generator used for IRQ 0
                case 3:                     // Square-Wave generator
                    p->counter = p->counter_max;
                break;

                default:
                    ERROR(("Unsupported PIT counting mode of %d", p->mode));
            }

            // We care only for counter 0, so start it if it just has been reprogrammed

            if( p == &pit[0] )
            {
                // Instead of calculating correct time delay for interrupts,
                // we will consider few common cases:
                //    DOS sets 0          -> 18.2 Hz
                //    Linux sets 2E9C     -> 100  Hz

                switch( p->counter_max )
                {
                    case 0:     // 18.2 Hz   DOS
                        p->DueTime.QuadPart = -549254;
                        break;

                    case 0x2E9C:
                        p->DueTime.QuadPart = -549254;          // Linux: we will keep DOS tick for now...
                        break;

                    default:
                        INFO(("Pit0 period set to %d Hz", 1193182 / p->counter_max));
                        INFO(("Pit0 time delay needs to be recalculated"));
                        ASSERT(0);
                }

//              p->DueTime.QuadPart =  -1000000;        // 1000000 100 ns (0.1 s)
//              p->DueTime.QuadPart = -10000000;        // 10000000 100 ns (1 s)

                // Restart the timer ticking...

                KeCancelTimer( &pDevExt->VmPitTimer0 );
                KeInsertQueueDpc(&pDevExt->VmPitDpc0, NULL, NULL);
            }
        }
    }
}

