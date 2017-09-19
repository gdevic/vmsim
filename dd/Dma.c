/******************************************************************************
*                                                                             *
*   Module:     Dma.c                                                         *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       6/27/2000                                                     *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the virtual DMA

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 6/27/2000  Original                                             Goran Devic *
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

#define DMA_MODE_DEMAND  0
#define DMA_MODE_SINGLE  1
#define DMA_MODE_BLOCK   2
#define DMA_MODE_CASCADE 3

typedef struct
{
    BOOL mask[4];
    BOOL flip_flop;
    BYTE status_reg;
    BYTE command_reg;
    BYTE request_reg;
    BYTE temporary_reg;
    BYTE extra[16];

    struct
    {
        struct
        {
            BYTE mode_type;
            BYTE address_decrement;
            BYTE autoinit_enable;
            BYTE transfer_type;

        } mode;

        WORD base_address;
        WORD base_count;
        WORD current_address;
        WORD current_count;
        BYTE page_reg;

    } chan[4];  // DMA channels 0..3

} TDma;

TDma dma;

BOOL TC = 0;                            // Terminal Count reached

#define s dma

// Index to find channel from register number (only [0],[1],[2],[6] used)

static BYTE channelindex[7] = {2, 3, 1, 0, 0, 0, 0};

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Dma_Initialize(void);

static DWORD Dma_fnIN(DWORD port, DWORD io_len);
static void Dma_fnOUT(DWORD port, DWORD data, DWORD io_len);

void DRQ(unsigned channel, BOOL val);
void set_TC(int value);
BOOL is_TC();
void raise_HLDA();
void floppy_dma_write(BYTE *data_byte);

/******************************************************************************
*                                                                             *
*   void Dma_Register(TVDD *pVdd)                                             *
*                                                                             *
*******************************************************************************
*
*   Registration function for the virtual DMA controller
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Dma_Register(TVDD *pVdd)
{
    pVdd->Initialize = Dma_Initialize;
    pVdd->Name = "DMA";
}


/******************************************************************************
*                                                                             *
*   int Dma_Initialize()                                                      *
*                                                                             *
*******************************************************************************
*
*   Initialized virtual DMA controller chip
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Dma_Initialize()
{
    int c;
    TRACE(("Dma_Initialize()"));

    RegisterIOPort(0x00, 0x0F, Dma_fnIN, Dma_fnOUT );   // Primary controller
    RegisterIOPort(0xC0, 0xDF, Dma_fnIN, Dma_fnOUT );   // Secondary controller
    RegisterIOPort(0x80, 0x8F, Dma_fnIN, Dma_fnOUT );   // Paging registers

    s.mask[0] = 1;                      // channel 0 masked
    s.mask[1] = 1;                      // channel 1 masked
    s.mask[2] = 1;                      // channel 2 masked
    s.mask[3] = 1;                      // channel 3 masked

    s.flip_flop = 0;                    // cleared
    s.status_reg = 0;                   // no requests, no terminal counts reached

    for (c=0; c<4; c++)
    {
        s.chan[c].mode.mode_type = 0;         // demand mode
        s.chan[c].mode.address_decrement = 0; // address increment
        s.chan[c].mode.autoinit_enable = 0;   // autoinit disable
        s.chan[c].mode.transfer_type = 0;     // verify
        s.chan[c].base_address = 0;
        s.chan[c].current_address = 0;
        s.chan[c].base_count = 0;
        s.chan[c].current_count = 0;
        s.chan[c].page_reg = 0;
    }

    return( 0 );
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Dma_fnIN(DWORD address, DWORD io_len)
{
    BYTE retval = 0;
    BYTE channel;

    TRACE(("Dma_fnIN(DWORD port=%04X, DWORD io_len=%X)", address, io_len));
    if( io_len!=1 )
        WARNING(("Not a BYTE access!"));

    switch (address)
    {
        case 0x00: /* DMA-1 current address, channel 0 */
        case 0x02: /* DMA-1 current address, channel 1 */
        case 0x04: /* DMA-1 current address, channel 2 */
        case 0x06: /* DMA-1 current address, channel 3 */
            channel = (BYTE) address >> 1;
            if (s.flip_flop==0) {
                s.flip_flop = !s.flip_flop;
                return(s.chan[channel].current_address & 0xff);
            }
            else {
                s.flip_flop = !s.flip_flop;
                return(s.chan[channel].current_address >> 8);
            }

        case 0x01: /* DMA-1 current count, channel 0 */
        case 0x03: /* DMA-1 current count, channel 1 */
        case 0x05: /* DMA-1 current count, channel 2 */
        case 0x07: /* DMA-1 current count, channel 3 */
            channel = (BYTE) address >> 1;
            if (s.flip_flop==0) {
                s.flip_flop = !s.flip_flop;
                return(s.chan[channel].current_count & 0xff);
            }
            else {
                s.flip_flop = !s.flip_flop;
                return(s.chan[channel].current_count >> 8);
            }

        case 0x08: // DMA-1 Status Register
            // bit 7: 1 = channel 3 request
            // bit 6: 1 = channel 2 request
            // bit 5: 1 = channel 1 request
            // bit 4: 1 = channel 0 request
            // bit 3: 1 = channel 3 has reached terminal count
            // bit 2: 1 = channel 2 has reached terminal count
            // bit 1: 1 = channel 1 has reached terminal count
            // bit 0: 1 = channel 0 has reached terminal count
            // reading this register clears lower 4 bits (terminal count flags)
            retval = s.status_reg;
            s.status_reg &= 0xf0;
            return(retval);
            break;
        case 0x0d: // dma-1: temporary register
            WARNING(("dma-1: read of temporary register\n"));
            // Note: write to 0x0D clears temporary register
            return(0);
            break;

        case 0x0081: // DMA-1 page register, channel 2
        case 0x0082: // DMA-1 page register, channel 3
        case 0x0083: // DMA-1 page register, channel 1
        case 0x0087: // DMA-1 page register, channel 0
            channel = channelindex[address - 0x81];
            return( s.chan[channel].page_reg );

        case 0x0080: // Extra page registers, temporary storage
        case 0x0084:
        case 0x0085:
        case 0x0086:
        case 0x0088:
        case 0x008C:
        case 0x008D:
        case 0x008E:
            retval = dma.extra[address-0x80];
            break;

        case 0x0089: // DMA-2 page register, channel 6
        case 0x008a: // DMA-2 page register, channel 7
        case 0x008b: // DMA-2 page register, channel 5
        case 0x008f: // DMA-2 page register, channel 4
            channel = channelindex[address - 0x89] + 4;
            WARNING(("dma: read: unsupported address=%04x (channel %d)\n", (unsigned) address, channel));
            break;

        case 0x00c0:
        case 0x00c2:
        case 0x00c4:
        case 0x00c6:
        case 0x00c8:
        case 0x00ca:
        case 0x00cc:
        case 0x00ce:
        case 0x00d0:
        case 0x00d2:
        case 0x00d4:
        case 0x00d6:
        case 0x00d8:
        case 0x00da:
        case 0x00dc:
        case 0x00de:
            WARNING(("dma: read: unsupported address=%04x\n", (unsigned) address));
            break;

        default:
            WARNING(("dma: read: unsupported address=%04x\n", (unsigned) address));
    }

    return(retval);
}

static void Dma_fnOUT(DWORD address, DWORD dvalue, DWORD io_len)
{
    BYTE set_mask_bit;
    BYTE channel;

    TRACE(("Dma_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", address, dvalue, io_len));

    if (io_len > 1)
    {
        if ( (io_len == 2) && (address == 0x0b) )
        {
            Dma_fnOUT(address,   dvalue & 0xff, 1);
            Dma_fnOUT(address+1, dvalue >> 8,   1);

            return;
        }

        WARNING(("dma: io write to address %08x, len=%u\n", (unsigned) address, (unsigned) io_len));
    }

    switch (address)
    {
        case 0x00:
        case 0x02:
        case 0x04:
        case 0x06:
            channel = (BYTE) address >> 1;
            if (s.flip_flop==0)
            {   // 1st byte
                s.chan[channel].base_address = (BYTE) dvalue;
                s.chan[channel].current_address = (BYTE) dvalue;
            }
            else
            {   // 2nd byte
                s.chan[channel].base_address |= (dvalue << 8);
                s.chan[channel].current_address |= (dvalue << 8);
            }
            s.flip_flop = !s.flip_flop;
            s.status_reg &= ~(1<<channel);       // Clear terminal count flag
            break;

        case 0x01:
        case 0x03:
        case 0x05:
        case 0x07:
            channel = (BYTE) address >> 1;
            if (s.flip_flop==0)
            {   // 1st byte
                s.chan[channel].base_count = (BYTE) dvalue;
                s.chan[channel].current_count = (BYTE) dvalue;
            }
            else
            {   // 2nd byte
                s.chan[channel].base_count |= (dvalue << 8);
                s.chan[channel].current_count |= (dvalue << 8);
            }
            s.flip_flop = !s.flip_flop;
            s.status_reg &= ~(1<<channel);       // Clear terminal count flag
            break;

        case 0x08: // DMA-1: command register
            if (dvalue != 0x04)
            {
                WARNING(("DMA: write to 0008: value(%02xh) not 04h\n", (unsigned) dvalue));
            }
            s.command_reg = (BYTE) dvalue;
            break;

        case 0x09: // DMA-1: request register
            // note: write to 0x0d clears this register
            if (dvalue & 0x04)
            {
                // set request bit
            }
            else
            {
                BYTE channel;

                // clear request bit
                channel = (BYTE) dvalue & 0x03;
                s.status_reg &= ~(1 << (channel+4));
            }
            break;

        case 0x0a:
            set_mask_bit = (BYTE) dvalue & 0x04;
            channel = (BYTE) dvalue & 0x03;
            s.mask[channel] = (set_mask_bit > 2);
            break;

        case 0x0b: // dma-1 mode register
            channel = (BYTE) dvalue & 0x03;
            s.chan[channel].mode.mode_type = ((BYTE) dvalue >> 6) & 0x03;
            s.chan[channel].mode.address_decrement = ((BYTE) dvalue >> 5) & 0x01;
            s.chan[channel].mode.autoinit_enable = ((BYTE) dvalue >> 4) & 0x01;
            s.chan[channel].mode.transfer_type = ((BYTE) dvalue >> 2) & 0x03;
            break;

        case 0x0c: // dma-1 clear byte flip/flop

            s.flip_flop = 0;
            break;

        case 0x0d: // dma-1: master disable
            /* ??? */
            // writing any value to this port resets dma controller 1
            // same action as a hardware reset
            // mask register is set (chan 0..3 disabled)
            // command, status, request, temporary, and byte flip-flop are all cleared
            s.mask[0] = 1;
            s.mask[1] = 1;
            s.mask[2] = 1;
            s.mask[3] = 1;
            s.command_reg = 0;
            s.status_reg = 0;
            s.request_reg = 0;
            s.temporary_reg = 0;
            s.flip_flop = 0;
            break;

        case 0x0e: // dma-1: clear mask register

            s.mask[0] = 0;
            s.mask[1] = 0;
            s.mask[2] = 0;
            s.mask[3] = 0;
            break;

        case 0x0f: // dma-1: write all mask bits

            s.mask[0] = dvalue & 0x01; dvalue >>= 1;
            s.mask[1] = dvalue & 0x01; dvalue >>= 1;
            s.mask[2] = dvalue & 0x01; dvalue >>= 1;
            s.mask[3] = dvalue & 0x01;
            break;

        case 0x81: // dma page register, channel 2
        case 0x82: // dma page register, channel 3
        case 0x83: // dma page register, channel 1
        case 0x87: // dma page register, channel 0
            // address bits A16-A23 for DMA channel
            channel = channelindex[address - 0x81];
            s.chan[channel].page_reg = (BYTE) dvalue;
            break;

        case 0x0080: // Extra page registers, temporary storage
        case 0x0084:
        case 0x0085:
        case 0x0086:
        case 0x0088:
        case 0x008C:
        case 0x008D:
        case 0x008E:
            dma.extra[address-0x80] = (BYTE) dvalue;
            break;

        case 0x00c0:
        case 0x00c2:
        case 0x00c4:
        case 0x00c6:
        case 0x00c8:
        case 0x00ca:
        case 0x00cc:
        case 0x00ce:
        case 0x00d0:
        case 0x00d2:
        case 0x00d4:
        case 0x00d6:
        case 0x00d8:
        case 0x00da:
        case 0x00dc:
        case 0x00de:
            WARNING(("DMA(ignored): write: %04xh = %04xh\n", (unsigned) address, (unsigned) dvalue));
            break;


        default:
            WARNING(("DMA(ignored): write: %04xh = %02xh\n", (unsigned) address, (unsigned) dvalue));
    }
}


DWORD Dma_GetAddress(DWORD channel)
{
    return( (s.chan[channel].page_reg << 16) | s.chan[channel].current_address );
}

DWORD Dma_GetOutstanding(DWORD channel)
{
    return( s.chan[channel].current_count + 1);
}

BOOL Dma_IsTC(DWORD channel)
{
    return(s.status_reg & (1<<channel));
}

void Dma_Roll(DWORD channel, WORD advance)
{
    // address increment
    if( s.chan[channel].current_count < advance )
    {
        // This advance terminates the dma transfer
        if (s.chan[channel].mode.autoinit_enable == 0)
        {
            // count expired, done with transfer
            s.chan[channel].current_address = advance - s.chan[channel].current_count - 1;
            s.chan[channel].current_count   = 0xFFFF;
            s.status_reg |= (1 << channel); // hold TC in status reg
        }
        else
        {
            ASSERT(0);  // We really cannot handle this since the transfer using
            // old address block has already occurred disregarding the new recycled
            // dma address..

            // count expired, but in autoinit mode
            // reload count and base address
            s.chan[channel].current_address = s.chan[channel].base_address;
            s.chan[channel].current_count = s.chan[channel].base_count;
        }
    }
    else
    {
        // It is safe to advance the counter

        s.chan[channel].current_address += advance;
        s.chan[channel].current_count -= advance;
    }
}


void Dma_Next(DWORD channel)
{
    s.chan[channel].current_address++;
    s.chan[channel].current_count--;

    if (s.chan[channel].current_count == 0xffff)
    {
        if (s.chan[channel].mode.autoinit_enable == 0)
        {
            // count expired, done with transfer
            s.status_reg |= (1 << channel); // hold TC in status reg
        }
        else
        {
            ASSERT(0);
            // count expired, but in autoinit mode
            // reload count and base address
            s.chan[channel].current_address = s.chan[channel].base_address;
            s.chan[channel].current_count = s.chan[channel].base_count;
        }
    }
}


void DRQ(unsigned channel, BOOL val)
{
    DWORD dma_base, dma_roof;

    if ( channel != 2 )
        ERROR(("bx_dma_c::DRQ(): channel %d != 2\n", channel));

    if (!val)
    {
        //bx_printf("bx_dma_c::DRQ(): val == 0\n");
        // clear bit in status reg
        // deassert HRQ if not pending DRQ's ?
        // etc.
        s.status_reg &= ~(1 << (channel+4));
        return;
    }

#if 1
    DbgPrint(("BX_DMA_THIS s.mask[2]: %02x\n", (unsigned) s.mask[2]));
    DbgPrint(("BX_DMA_THIS s.flip_flop: %u\n", (unsigned) s.flip_flop));
    DbgPrint(("BX_DMA_THIS s.status_reg: %02x\n", (unsigned) s.status_reg));
    DbgPrint(("mode_type: %02x\n", (unsigned) s.chan[channel].mode.mode_type));
    DbgPrint(("address_decrement: %02x\n", (unsigned) s.chan[channel].mode.address_decrement));
    DbgPrint(("autoinit_enable: %02x\n", (unsigned) s.chan[channel].mode.autoinit_enable));
    DbgPrint(("transfer_type: %02x\n", (unsigned) s.chan[channel].mode.transfer_type));
    DbgPrint((".base_address: %04x\n", (unsigned) s.chan[channel].base_address));
    DbgPrint((".current_address: %04x\n", (unsigned) s.chan[channel].current_address));
    DbgPrint((".base_count: %04x\n", (unsigned) s.chan[channel].base_count));
    DbgPrint((".current_count: %04x\n", (unsigned) s.chan[channel].current_count));
    DbgPrint((".page_reg: %02x\n", (unsigned) s.chan[channel].page_reg));
#endif

    s.status_reg |= (1 << (channel+4));

    //  if (BX_DMA_THIS s.mask[channel])
    //    bx_panic("bx_dma_c::DRQ(): BX_DMA_THIS s.mask[] is set\n");


    if ( (s.chan[channel].mode.mode_type != DMA_MODE_SINGLE) &&
        (s.chan[channel].mode.mode_type != DMA_MODE_DEMAND) )
        ERROR(("bx_dma_c::DRQ: mode_type(%02x) not handled\n", (unsigned) s.chan[channel].mode.mode_type));
    if (s.chan[channel].mode.address_decrement != 0)
        ERROR(("bx_dma_c::DRQ: address_decrement != 0\n"));
    //if (BX_DMA_THIS s.chan[channel].mode.autoinit_enable != 0)
    //  bx_panic("bx_dma_c::DRQ: autoinit_enable != 0\n");

    dma_base = (s.chan[channel].page_reg << 16) | s.chan[channel].base_address;
    dma_roof = dma_base + s.chan[channel].base_count;
    if ( (dma_base & 0xffff0000) != (dma_roof & 0xffff0000) ) {
        DbgPrint(("dma_base = %08x\n", (unsigned) dma_base));
        DbgPrint(("dma_base_count = %08x\n", (unsigned) s.chan[channel].base_count));
        DbgPrint(("dma_roof = %08x\n", (unsigned) dma_roof));
        DbgPrint(("dma: DMA request outside 64k boundary\n"));
    }

    //bx_printf("DRQ set up for single mode, increment, auto-init disabled, write\n");
    // should check mask register VS DREQ's in status register here?
    // assert Hold ReQuest line to CPU
//    bx_pc_system.set_HRQ(1);
}

void set_TC(int value)
{
    TC = value;
}

BOOL is_TC()
{
    return( TC );
}

void raise_HLDA()
{
#if 0
    unsigned channel;
    DWORD phy_addr;
    BOOL count_expired = 0;

    // find highest priority channel
    for (channel=0; channel<4; channel++) {
        if ( (s.status_reg & (1 << (channel+4))) &&
            (s.mask[channel]==0) ) {
            break;
        }
    }
    if (channel >= 4) {
        // don't panic, just wait till they're unmasked
        //    bx_panic("hlda: no unmasked requests\n");
        return;
    }

    //bx_printf("hlda: OK in response to DRQ(%u)\n", (unsigned) channel);
    phy_addr = (s.chan[channel].page_reg << 16) | s.chan[channel].current_address;

//    bx_pc_system.set_DACK(channel, 1);

    // check for expiration of count, so we can signal TC and DACK(n)
    // at the same time.

    if (s.chan[channel].mode.address_decrement==0)
    {
        // address increment
        s.chan[channel].current_address++;
        s.chan[channel].current_count--;
        if (s.chan[channel].current_count == 0xffff)
            if (s.chan[channel].mode.autoinit_enable == 0)
            {
                // count expired, done with transfer
                // assert TC, deassert HRQ & DACK(n) lines
                s.status_reg |= (1 << channel); // hold TC in status reg
                set_TC(1);
                count_expired = 1;
            }
            else {
                // count expired, but in autoinit mode
                // reload count and base address
                s.chan[channel].current_address = s.chan[channel].base_address;
                s.chan[channel].current_count = s.chan[channel].base_count;
            }
    }
    else {
        // address decrement
        ERROR(("hlda: decrement not implemented\n"));
    }

    if (s.chan[channel].mode.transfer_type == 1)
    { // write
        // xfer from I/O to Memory
        if( channel==2 )
            floppy_dma_write((BYTE *) phy_addr);
        else
            ASSERT(0);
//        pc_sys->dma_write8(phy_addr, channel);
    }
    else if (s.chan[channel].mode.transfer_type == 2)
    { // read
        // xfer from Memory to I/O
//        pc_sys->dma_read8(phy_addr, channel);
ASSERT(0);
    }
    else {
        ERROR(("hlda: transfer_type of %u not handled\n",(unsigned) s.chan[channel].mode.transfer_type));
    }

    if (count_expired)
    {
//        set_TC(0);            // clear TC, adapter card already notified
//        set_HRQ(0);           // clear HRQ to CPU
//        set_DACK(channel, 0); // clear DACK to adapter card
    }
#endif
}

