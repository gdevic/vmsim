/******************************************************************************
*                                                                             *
*   Module:     Floppy.c                                                      *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       6/22/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for virtual floppy device.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 6/22/2000  Original                                             Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Include Files                                                             *
******************************************************************************/
#include <strmini.h>                // Include driver level C functions

#include <ntddk.h>                  // Include ddk function header file

#include "Vm.h"                     // Include root header file

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

#define FLOPPY_CONTROLLER_BASE  0x3F0

typedef struct
{
    BYTE DOR;
    BYTE Status;                        // Main Status Register       (3F4/374)
    BYTE DIR;                           // Digital Input Register     (3F7/377)

} TController;

typedef struct
{
    BYTE Command[10];                   // Command data array
    BYTE Command_Index;                 // Current data to write
    BYTE Command_Size;                  // Total expected command size

    BYTE Result[10];                    // Result data array
    BYTE Result_Index;                  // Current data to read
    BYTE Result_Size;                   // Total size of the result

    DWORD track;                        // Current track
    DWORD head;                         // Current head
    DWORD sector;                       // Current sector

    DWORD Command0;                     // Used with transfer: pending command
    DWORD max_sector;                   // Used with transfer: final sector
    DWORD logical_sector;               // Used with transfer: current logical

} TFloppy;

TController control;
TFloppy     floppy[2];

// Main status register

#define FD_MS_MRQ  0x80
#define FD_MS_DIO  0x40
#define FD_MS_NDMA 0x20
#define FD_MS_BUSY 0x10
#define FD_MS_ACTD 0x08
#define FD_MS_ACTC 0x04
#define FD_MS_ACTB 0x02
#define FD_MS_ACTA 0x01

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Floppy_Initialize(void);

static DWORD Floppy_fnIN(DWORD port, DWORD io_len);
static void Floppy_fnOUT(DWORD port, DWORD data, DWORD io_len);

static void Command_Complete(TFloppy *p);
static void Floppy_Command( TFloppy *p );
static void Floppy_Transfer(VMREQUEST *pVmReq, DWORD pArg, BYTE *pBuffer);
static void Command_Reset();

/******************************************************************************
*                                                                             *
*   void Floppy_Register(TVDD *pVdd)                                          *
*                                                                             *
*******************************************************************************
*
*   Registration function for the virtual floppy device.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Floppy_Register(TVDD *pVdd)
{
    pVdd->Initialize = Floppy_Initialize;
    pVdd->Name = "Floppy";
}


/******************************************************************************
*                                                                             *
*   int Floppy_Initialize()                                                   *
*                                                                             *
*******************************************************************************
*
*   Initializes virtual floppy controller device interface as a primary
*   controller.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Floppy_Initialize()
{
    TRACE(("Floppy_Initialize()"));

    memset(floppy, 0, sizeof(floppy));

    // Register primary floppy controller

    RegisterIOPort(0x3F2, 0x3F7, Floppy_fnIN, Floppy_fnOUT );

    // Initialize floppy controller

    control.DOR = 0x0C;               // Enable DMA and controller
    control.DIR = 0x80;               // Disk changed

    // Reset floppy device

    Command_Reset();        // Call soft reset function

    return( 0 );
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************
static DWORD Floppy_fnIN(DWORD port, DWORD io_len)
{
    DWORD value = 0xFF;
    TFloppy *p;
    TRACE(("Floppy_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));

    ASSERT(io_len==1);

    // Select the disk that receives attention
    p = &floppy[ control.DOR & 1 ];

    switch( port - FLOPPY_CONTROLLER_BASE )
    {
        case 2:                         // Digital Output Register
                value = control.DOR;
            break;

        case 4:                         // Main Status Register
                value = control.Status;
            break;

        case 5:                         // Data Register
                value = p->Result[p->Result_Index++];
                if( p->Result_Index >= p->Result_Size )
                {
                    p->Result_Index = p->Result_Size = 0;
                    control.Status = FD_MS_MRQ;
                }
            break;

        case 7:                         // Digital Input Register
                // This port is shared with the hard drive controller
                // Only bit 7 of DIR is used
//                value = hdd();
                value = (value & 0x7F) | (control.DIR & 0x80);
            break;

        default:
            WARNING(("Invalid port read floppy+%d", port));
    }

    return( value );
}


static void Floppy_fnOUT(DWORD port, DWORD value, DWORD io_len)
{
    TFloppy *p;

    TRACE(("Floppy_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, value, io_len));

    ASSERT(io_len==1);

    // Select the disk that receives attention
    p = &floppy[ control.DOR & 1 ];

    switch( port - FLOPPY_CONTROLLER_BASE )
    {
        case 2:                         // Digital Output Register
                // 7:4  MOTD:MOTA Motor on select
                // 3    DMA enable                  -> 1
                // 2    ~RESET                      -> 1
                // 1:0  DR1:DR0 Drive select
                control.DOR = (BYTE) value;
                ASSERT((control.DOR & 3)== 0);  // I want to see when he's going to
                                                // access second floppy........

                if( (control.DOR & 4)==0 )   // Execute soft reset if reset is held down
                    Command_Reset();
            break;

        case 4:                         // Write: Data Rate Select Register
            break;

        case 5:                         // Data Register
            p->Command[p->Command_Index++] = (BYTE) value;

            if( p->Command_Size==0 )    // Ready to accept a new command
            {
                switch( value )
                {
                case 0x03:  p->Command_Size = 3; break;   // Specify
                case 0x04:  p->Command_Size = 2; break;   // Get Status
                case 0x07:  p->Command_Size = 2; break;   // Recalibrate
                case 0x0F:  p->Command_Size = 3; break;   // Seek
                case 0x4A:  p->Command_Size = 2; break;   // Read ID
                case 0xC5:  p->Command_Size = 9; break;   // Write normal data
                case 0xE6:  p->Command_Size = 9; break;   // Read normal data
                case 0x13:  p->Command_Size = 3; break;   // Configure (enhanced)
                case 0x0E:  p->Command_Size = 3; break;   // Configure (enhanced)
                default:
                    ERROR(("Unsupported floppy command %02X", value));
                }
                control.Status = FD_MS_MRQ | FD_MS_BUSY;
            }
            else
            if( p->Command_Index >= p->Command_Size )
            {
                // This value completes the command

                Floppy_Command( p );

                p->Command_Index = p->Command_Size = 0;
            }
            break;

        case 6:                         // Used with hdd controller
            // This port is shared with the hard drive controller
            break;

        case 7:                         // Configuration Control Register
            break;

        default:
            WARNING(("Invalid port write floppy+%d", port));
    }
}

// This function should be called with a delay to simulate time it takes
// for floppy to execute a command.  However, it works even if it is not
// called that way, but directly.. we'll see how long it will work......

static void Command_Complete(TFloppy *p)
{
    switch( p->Command0 )
    {
        case 0x07:              // Delayed command: Recalibrate
                p->track = 0;
        case 0x0F:              // Delayed command: Seek
                control.Status = FD_MS_MRQ;
                control.DIR = 0x00;
                VmSIMGenerateInterrupt(6);
            break;

        case 0x4A:              // Delayed command: Read sector ID
        case 0xC5:              // Delayed command: Write normal data
        case 0xE6:              // Delayed command: Read normal data
                control.Status = FD_MS_MRQ | FD_MS_DIO;
                VmSIMGenerateInterrupt(6);
            break;

        case 0xFE:              // Delayed command: Reset
                Command_Reset();
            break;

        default:
            ERROR(("Invalid complition command %02X", p->Command0));
    }
}

// This function parses a command and executes steps needed for it

static void Floppy_Command( TFloppy *p )
{
    static VMREQUEST VmReq;
    DWORD track, head, sector;

    p->Result_Size = 0;
    p->Result_Index = 0;
    p->Command0 = p->Command[0];

    switch( p->Command[0] )
    {
        case 0x03:          // Command: Specify
        case 0x13:          // Command: Configure drive parameters
                control.Status = FD_MS_MRQ;
            break;

        case 0x04:          // Command: Get Status
                p->Result[0] = 0x00;
                p->Result_Size = 1;
                control.Status = FD_MS_MRQ | FD_MS_DIO | FD_MS_BUSY;
            break;

        case 0x07:          // Command: Recalibrate
                control.Status = FD_MS_DIO | FD_MS_BUSY;
                Command_Complete( p );
            break;

        case 0x08:          // Command: Sense Interrupt Status
                p->Result[0] = 0x20 | (control.DOR & 3);
                p->Result[1] = (BYTE) p->track;
                p->Result_Size = 2;
                control.Status = FD_MS_MRQ | FD_MS_DIO | FD_MS_BUSY;
            break;

        case 0x0F:          // Command: Seek
                p->head = (p->Command[1] >> 2 ) & 1;
                p->track = p->Command[2];
                control.Status = FD_MS_DIO | FD_MS_BUSY;
                Command_Complete( p );
            break;

        case 0x4A:          // Command: Read sector ID
                p->Result[0] = 0;
                p->Result[1] = 0;
                p->Result[2] = 0;
                p->Result[3] = (BYTE) p->track;
                p->Result[4] = (BYTE) p->head;
                p->Result[5] = (BYTE) p->sector;
                p->Result[6] = 2;
                control.Status = FD_MS_MRQ | FD_MS_DIO | FD_MS_BUSY;
                Command_Complete( p );
            break;

        case 0xE6:          // Command: Read normal data
        case 0xC5:          // Command: Write normal data
                p->Result_Size = 7;
                control.Status = FD_MS_MRQ | FD_MS_BUSY;

                track  = p->Command[2];     // 0..79
                head   = p->Command[3] & 1; // 0..1
                sector = p->Command[4];     // 1..36
                p->max_sector = p->Command[6];
                if( p->max_sector==0 || p->max_sector>MAX_FLOPPY_SECTORS )
                    p->max_sector = MAX_FLOPPY_SECTORS;

                p->logical_sector = (track * MAX_FLOPPY_HEADS * MAX_FLOPPY_SECTORS) +
                                    (head                     * MAX_FLOPPY_SECTORS) +
                                    (sector - 1);

                // Position the head over the starting coordinates
                p->track  = track;
                p->head   = head;
                p->sector = sector;

                // READ:
                // We request all sectors at once - app allocates a buffer
                // that is large enough, fills it in and sends it down to us so the
                // read completion routine can copy it to VM memory.
                //
                // WRITE:
                // First, we send a request for an empty buffer in which we
                // write our sectors from the write completion routine and send back.

                VmReq.head.type        = p->Command[0]==0xE6? ReqVMReadDisk : ReqVMWriteDisk;
                VmReq.head.flags       = Dma_GetAddress(2);
                VmReq.head.bufferlen   = 0;
                VmReq.head.pComplete   = Floppy_Transfer;
                VmReq.head.completeArg = (DWORD) p;
                VmReq.head.drive       = (p==&floppy[0])? 0 : 1;
                VmReq.head.offset      = p->logical_sector * 512;
                VmReq.head.sectors     = p->max_sector - p->sector + 1;

                // Do not transfer more than the DMA has been set up to transfer
                if( Dma_GetOutstanding(2) < (VmReq.head.sectors * 512) )
                    VmReq.head.sectors = Dma_GetOutstanding(2) / 512;

                SendRequestToGui( &VmReq );
            break;

        default:
            ERROR(("Invalid floppy command of %02X", p->Command[0]));
    }
}


// Completion function for floppy read operation - a buffer containing
// the total number of sectors is returned from the app, filled in with
// the sector data.
//    AND
// Completion function for floppy write operation - an empty buffer
// is returned from the app in which we deposit our sectors to
// be written when this function returns it to the app.

static void Floppy_Transfer(VMREQUEST *pVmReq, DWORD pArg, BYTE *pBuffer)
{
    DWORD dwVmPhys;
    TFloppy *p = (TFloppy *) pArg;
    ASSERT(pBuffer);

    while( pVmReq->head.sectors && !Dma_IsTC(2) && (p->sector <= p->max_sector))
    {
        dwVmPhys = Dma_GetAddress(2);

        if( p->Command0 == 0xE6 )       // READ disk
        {
            RtlMoveMemory((void *) (Mem.MemVM.linear + dwVmPhys), pBuffer, 512),
            MetaInvalidateVmPhysAddress( Dma_GetAddress(2) );
        }
        else                            // WRITE to disk
            RtlMoveMemory(pBuffer, (void *)(Mem.MemVM.linear + dwVmPhys), 512);

        Dma_Roll(2, 512);

        // Increment logical and physical sector
        p->logical_sector++;
        if( p->sector++ > MAX_FLOPPY_SECTORS )
        {
            p->sector = 1;
            if( p->head++ >= MAX_FLOPPY_HEADS )
            {
                p->head = 0;
                if( p->track >= MAX_FLOPPY_TRACKS )
                    p->track = MAX_FLOPPY_TRACKS,
                    p->logical_sector--;
            }
        }

        pBuffer += 512;
        pVmReq->head.sectors--;
    }

    // Finish data transfer, signal end-of-transfer

    p->Result[0] = ((BYTE)p->head << 2) | 0x20; // ST0
    p->Result[1] = 0;                           // ST1
    p->Result[2] = 0;                           // ST2
    p->Result[3] = (BYTE) p->track;
    p->Result[4] = (BYTE) p->head;
    p->Result[5] = (BYTE) p->sector;
    p->Result[6] = 2;                           // Sector size

    Command_Complete( p );
}

// Soft reset of the floppy controller

static void Command_Reset()
{
    int i;
    for( i=0; i<2; i++ )
    {
        floppy[i].Command_Size = 0;
        floppy[i].Command_Index = 0;
        floppy[i].Result_Index = 0;
        floppy[i].Result_Size = 0;
        floppy[i].track = 0;
        floppy[i].head = 0;
        floppy[i].sector = 1;
    }
}
