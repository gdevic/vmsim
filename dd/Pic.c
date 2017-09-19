/******************************************************************************
*                                                                             *
*   Module:     Pic.c														  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       5/10/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for the virtual Programmable Interrupt
		Controller.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 5/10/2000	 Original                                             Goran Devic *
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

typedef struct
{
    BYTE irr;                   // Interrupt request register
    BYTE isr;                   // In-service register
    BYTE imr;                   // Interrupt mask register
    BYTE mode;                  // ICW4, mode bits

    BYTE InterruptOffset;       // [7:3] interrupt offset

    DWORD fReadISR;             // 1=read isr, 0=read irr register
    DWORD fInInit;              // 2, 3, 4 - in initialization phase
    DWORD fRequires4;           // Init: requires ICW4

} TPIC1;

static TPIC1 pic[2];            // 2 PICs (Master and Slave)

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int Pic_Initialize(void);

static DWORD Pic_fnIN(DWORD port, DWORD io_len);
static void Pic_fnOUT(DWORD port, DWORD data, DWORD io_len);
static int ProbePicState();


/******************************************************************************
*                                                                             *
*   void Pic_Register(TVDD *pVdd)                                             *
*                                                                             *
*******************************************************************************
*
*   Registration function for the PIC.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void Pic_Register(TVDD *pVdd)
{
    pVdd->Initialize = Pic_Initialize;
    pVdd->Name = "PIC";
}


/******************************************************************************
*                                                                             *
*   int Pic_Initialize()                                                      *
*                                                                             *
*******************************************************************************
*
*   Initializes virtual PIC
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int Pic_Initialize()
{
    TRACE(("Pic_Initialize()"));

    memset(&pic[0], 0, sizeof(TPIC1));
    memset(&pic[1], 0, sizeof(TPIC1));

    pic[0].imr = 0xFF;          // All IRQs initially masked
    pic[0].InterruptOffset = 0x08;
    pic[1].imr = 0xFF;
    pic[1].InterruptOffset = 0x70;

    RegisterIOPort(0x20, 0x21, Pic_fnIN, Pic_fnOUT );
    RegisterIOPort(0xA0, 0xA1, Pic_fnIN, Pic_fnOUT );

    return( 0 );
}



/******************************************************************************
*                                                                             *
*   int VmSIMGenerateInterrupt(DWORD IRQ_number)                              *
*                                                                             *
*******************************************************************************
*
*   This function asserts a specific IRQ interrupt to the PIC.
*
*   Where:
*       IRQ_number is the IRQ to assert: 0-15 excluding 2
*
*   Returns:
*       0
*       1 bad IRQ_number
*
******************************************************************************/
int VmSIMGenerateInterrupt(DWORD IRQ_number)
{
    TPIC1 *p;
    int mask;

//    TRACE(("VmSIMGenerateInterrupt(DWORD IRQ_number=0x%02X)", IRQ_number));

    if( (IRQ_number <= 15) && (IRQ_number != 2) )
    {
        p = &pic[ (IRQ_number >> 3) & 1];   // Master: 0-7, Slave: 8-15

        p->irr |= 1 << (IRQ_number & 7);    // Set the request register

        // Send a signal to the MainLoop.c if an interrupt is waiting to happen

        CPU.fInterrupt = ProbePicState();

        return( 0 );
    }
    ERROR(("Bad IRQ number of %02X",IRQ_number));

    return( 1 );
}


/******************************************************************************
*                                                                             *
*   BYTE Pic_Ack()                                                            *
*                                                                             *
*******************************************************************************
*
*   CPU acknowledges the interrupt and is about to service it.  It just needs
*   to call this function to:
*       1) Get the interrupt number to service
*       2) Acknowledge the interrupt with the virtual PIC
*
*   Returns:
*       Interrupt number (from the InterruptOffset field)
*
******************************************************************************/
BYTE Pic_Ack()
{
    int irq;
    TPIC1 *p;

    irq = ProbePicState() - 1;

    p = &pic[ ( irq >> 3) & 1];  // Selects 0/1 from 0-7/8-15

    // Reset the need for an interrupt since we are going to service it

    CPU.fInterrupt = 0;

    // Set the interrupt-in-service bit and reset request bit

    p->isr |= 1 << irq;
    p->irr &= ~(1 << irq);

    // Return the software interrupt number. The base is stored in InterruptOffset

    return( p->InterruptOffset + irq );
}


/******************************************************************************
*                                                                             *
*   int ProbePicState()                                                       *
*                                                                             *
*******************************************************************************
*
*   Probes the new state of PIC chips for a possible condition that would
*   raise the interrupt signal.
*
*   Returns:
*       0 if no IRQ should be serviced
*       (IRQ + 1) that should be serviced
*
******************************************************************************/
static int ProbePicState()
{
    int irq = -1;

    if( (pic[0].irr & 0x01) && ((pic[0].isr & 0x01)==0) && ((pic[0].imr & 0x01)==0) )  irq = 0;
    else
    if( (pic[0].irr & 0x02) && ((pic[0].isr & 0x02)==0) && ((pic[0].imr & 0x02)==0) )  irq = 1;
    else
    if( (pic[1].irr & 0x01) && ((pic[1].isr & 0x01)==0) && ((pic[1].imr & 0x01)==0) )  irq = 8;
    else
    if( (pic[1].irr & 0x02) && ((pic[1].isr & 0x02)==0) && ((pic[1].imr & 0x02)==0) )  irq = 9;
    else
    if( (pic[1].irr & 0x04) && ((pic[1].isr & 0x04)==0) && ((pic[1].imr & 0x04)==0) )  irq = 10;
    else
    if( (pic[1].irr & 0x08) && ((pic[1].isr & 0x08)==0) && ((pic[1].imr & 0x08)==0) )  irq = 11;
    else
    if( (pic[1].irr & 0x10) && ((pic[1].isr & 0x10)==0) && ((pic[1].imr & 0x10)==0) )  irq = 12;
    else
    if( (pic[1].irr & 0x20) && ((pic[1].isr & 0x20)==0) && ((pic[1].imr & 0x20)==0) )  irq = 13;
    else
    if( (pic[1].irr & 0x40) && ((pic[1].isr & 0x40)==0) && ((pic[1].imr & 0x40)==0) )  irq = 14;
    else
    if( (pic[1].irr & 0x80) && ((pic[1].isr & 0x80)==0) && ((pic[1].imr & 0x80)==0) )  irq = 15;
    else
    if( (pic[0].irr & 0x04) && ((pic[0].isr & 0x04)==0) && ((pic[0].imr & 0x04)==0) )  irq = 2;
    else
    if( (pic[0].irr & 0x08) && ((pic[0].isr & 0x08)==0) && ((pic[0].imr & 0x08)==0) )  irq = 3;
    else
    if( (pic[0].irr & 0x10) && ((pic[0].isr & 0x10)==0) && ((pic[0].imr & 0x10)==0) )  irq = 4;
    else
    if( (pic[0].irr & 0x20) && ((pic[0].isr & 0x20)==0) && ((pic[0].imr & 0x20)==0) )  irq = 5;
    else
    if( (pic[0].irr & 0x40) && ((pic[0].isr & 0x40)==0) && ((pic[0].imr & 0x40)==0) )  irq = 6;
    else
    if( (pic[0].irr & 0x80) && ((pic[0].isr & 0x80)==0) && ((pic[0].imr & 0x80)==0) )  irq = 7;

    return( irq + 1 );
}

//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD Pic_fnIN(DWORD port, DWORD io_len)
{
    TPIC1 *p;

    TRACE(("Pic_fnIN(DWORD port=%04X, DWORD io_len=%X)", port, io_len));
    ASSERT(io_len==1);

    p = &pic[ (port >> 8) & 1];  // Selects 0/1 from 0x20/0xA0

    switch( port )
    {
        case 0x20:          // Read irr/isr
        case 0xA0:
                if( p->fReadISR )
                    return( p->isr );
                else
                    return( p->irr );

        case 0x21:          // Read imr
        case 0xA1:
                return( p->imr );
    }

    ASSERT(0);
    return( 0 );
}

static void Pic_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    TPIC1 *p;
    int mask;

    TRACE(("Pic_fnOUT(DWORD port=%04X, DWORD data=%X, DWORD io_len=%X)", port, data, io_len));
    ASSERT(io_len==1);

    p = &pic[ (port >> 8) & 1];  // Selects 0/1 from 0x20/0xA0

    switch( port )
    {
        case 0x20:          // ICW1 / OCW2 / OCW 3
        case 0xA0:
            if( data & 0x10 )           // ICW1
            {
                INFO(("PIC ICW1: %02X", data));
                p->fInInit = 2;         // Expecting init. ICW2
                p->imr = 0xFF;
                p->isr = 0;
                p->irr = 0;

                if( data & 2 ) WARNING(("SNGL[1] should not be set!"));
                if( data & 1 ) p->fRequires4 = 1;

                return;
            }

            switch( data )
            {
                case 0x0A:  p->fReadISR = 0;  break;
                case 0x0B:  p->fReadISR = 1;  break;

                case 0x20:  // Non-specific EOI - clear highest current in-service bit
                    for( mask=0x01; mask!=0x100; mask<<=1 )
                        if( p->isr & mask )
                        {
                            p->isr &= ~mask;
                            break;
                        }
                    CPU.fInterrupt = ProbePicState();
                    break;

                case 0x60:  // Specific EOI
                case 0x61:  // Specific EOI
                case 0x62:  // Specific EOI
                case 0x63:  // Specific EOI
                case 0x64:  // Specific EOI
                case 0x65:  // Specific EOI
                case 0x66:  // Specific EOI
                case 0x67:  // Specific EOI
                    p->isr &= ~(1 << (data-0x60));      // Clear in service bit
                    CPU.fInterrupt = ProbePicState();
                    break;

                default:
                    ASSERT(0);
            }
            break;

        case 0x21:          // ICW2 - ICW4 and OCW1
        case 0xA1:
            switch( p->fInInit )
            {
                case 0:         // Not in init mode.  Normal operation.
                    INFO(("Interrupt mask register: %02X", data));
                    p->imr = (BYTE) data;
                break;

                case 2:                 // Expect ICW2
                    INFO(("ICW2: Interrupt offset: %02X", data));
                    p->InterruptOffset = (BYTE) data & 0xF8;
                    p->fInInit = 3;
                break;

                case 3:                 // Expect ICW3
                    INFO(("ICW3: Ignored"));
                    // Ignore this init byte
                    p->fInInit = p->fRequires4 ? 4 : 0;
                break;

                case 4:                 // Optional ICW4
                    INFO(("ICW4: Mode: %02X", data));
                    p->mode = (BYTE) data;
                    p->fInInit = 0;
                break;
            }
            break;
    }
}

