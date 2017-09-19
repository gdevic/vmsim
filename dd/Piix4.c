/******************************************************************************
*                                                                             *
*   Module:     PIIX4.c														  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/24/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for (virtual) Intel PIIX4 chipset:

        PIIX4 is a multifunction PCI device, for the total of 4, all of which
        only device 7110 (PCI-to-ISA Bridge) and 7111 (IDE Controller) are
        actually needed right now.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/24/2000	 Original                                             Goran Devic *
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
// Save previous packing value and set byte-packing
#pragma pack( push, piix4 )
#pragma pack( 1 )


static TPciHeader PCI_7110;     // Config space for PIIX4 function 0

typedef struct
{
    BYTE    res0[12];
    BYTE    ior;
    BYTE    res1[1];
    WORD    xbcs;
    BYTE    res2[16];
    DWORD   pirqrc;
    BYTE    serirqc;
    BYTE    res3[4];
    BYTE    tom;
    WORD    mstat;
    BYTE    res4[10];
    WORD    mbdma;
    BYTE    res5[8];
    BYTE    apicbase;
    BYTE    res6[1];
    BYTE    dlc;
    BYTE    res7[13];
    WORD    pdmacfg;
    DWORD   ddmabase;
    BYTE    res8[26];
    DWORD   gencfg;
    BYTE    res9[23];
    BYTE    rtccfg;
    BYTE    resA[52];
} T7110;

static TPciHeader PCI_7111;     // Config space for PIIX4 function 1

typedef struct
{
    DWORD   idetm;
    BYTE    sidetm;
    BYTE    res1[3];
    BYTE    udmactl;
    BYTE    res2[1];
    WORD    udmatim;
    BYTE    res3[180];
} T7111;


typedef struct
{
    BYTE port61;
} TPixMisc;


TPixMisc Pix;


// Restore packing value
#pragma pack( pop, piix4 )

/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int PIIX4_Initialize(void);

static DWORD Piix4_PciFnIN(DWORD function, DWORD index);
static void Piix4_PciFnOUT(DWORD function, DWORD index, DWORD data);

static DWORD Piix4_fnIN(DWORD port, DWORD io_len);
static void Piix4_fnOUT(DWORD port, DWORD data, DWORD io_len);

/******************************************************************************
*                                                                             *
*   void PIIX4Register(TVDD *pVdd)                                            *
*                                                                             *
*******************************************************************************
*
*   Registration function for the PIIX4 chipset emulation.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void PIIX4_Register(TVDD *pVdd)
{
    pVdd->Initialize = PIIX4_Initialize;
    pVdd->Name = "PIIX4";
}


/******************************************************************************
*                                                                             *
*   int PIIX4_Initialize()                                                    *
*                                                                             *
*******************************************************************************
*
*   Initialization function for the PIIX4 chipset emulation.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int PIIX4_Initialize()
{
    TPciHeader * pPCI;
    T7110 *pPCI7110;
    T7111 *pPCI7111;

    TRACE(("PIIX4_Initialize()"));

    ASSERT(sizeof(T7110)==192);
    ASSERT(sizeof(T7111)==192);

    // Register device #7 to be PIIX4 multifunction device

    RegisterPCIDevice( 7, Piix4_PciFnIN, Piix4_PciFnOUT );

    // Register ports

    RegisterIOPort(0x61, 0x61, Piix4_fnIN, Piix4_fnOUT );
    Pix.port61 = 0x10;

    // Initialize PIIX4 PCI config space registers with their initial values

    pPCI = &PCI_7110;                           // PCI to ISA bridge
    pPCI7110 = (T7110*) pPCI->deviceSpecific;
    RtlZeroMemory(pPCI, sizeof(TPciHeader));

    pPCI->vendorID      = 0x8086;
    pPCI->deviceID      = 0x7110;
    pPCI->command       = 0x0007;
    pPCI->status        = 0x0280;
    pPCI->revisionClass = 0x06010000;
    pPCI->headerType    = 0x80;
    pPCI7110->ior       = 0x4D;
    pPCI7110->xbcs      = 0x03;
    pPCI7110->pirqrc    = 0x80;
    pPCI7110->serirqc   = 0x10;
    pPCI7110->tom       = 0x02;
    pPCI7110->mstat     = 0x00;
    pPCI7110->mbdma     = 0x04;
    pPCI7110->apicbase  = 0x00;
    pPCI7110->dlc       = 0x00;
    pPCI7110->pdmacfg   = 0x00;
    pPCI7110->ddmabase  = 0x00;
    pPCI7110->gencfg    = 0x00;
    pPCI7110->rtccfg    = 0x21;

    pPCI = &PCI_7111;                           // PCI to ISA bridge
    pPCI7111 = (T7111*) pPCI->deviceSpecific;
    RtlZeroMemory(pPCI, sizeof(TPciHeader));

    pPCI->vendorID      = 0x8086;
    pPCI->deviceID      = 0x7110;
    pPCI->command       = 0x0007;
    pPCI->status        = 0x0280;
    pPCI->revisionClass = 0x06010000;
    pPCI->headerType    = 0x80;
    pPCI7111->idetm     = 0x00;
    pPCI7111->sidetm    = 0x00;
    pPCI7111->udmactl   = 0x00;
    pPCI7111->udmatim   = 0x00;

    return( 0 );
}


//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// PCI registers
//-----------------------------------------------------------------------------
static DWORD Piix4_PciFnIN(DWORD function, DWORD index)
{
    DWORD value;
    TRACE(("Piix4_PciFnIN(function=%04X, index=%X)", function, index));

    if( function==0 )       // PIIX4 function 0
    {
        value = *(int *)(((BYTE *)&PCI_7110) + index);
    }
    else
    if( function==1 )       // PIIX4 function 1
    {
        value = *(int *)(((BYTE *)&PCI_7111) + index);
    }
    else
    {
        ASSERT(0);          // Invalid function device
    }

    TRACE(("Returns: %04X", value));
    return( value );
}


// TODO : CHECK FOR READ ONLY FIELDS

static void Piix4_PciFnOUT(DWORD function, DWORD index, DWORD data)
{
    TRACE(("Piix4_PciFnOUT(function=%04X, index=%X, data=%X)", function, index, data));

    if( function==0 )       // PIIX4 function 0
    {
        *(int *)(((BYTE *)&PCI_7110) + index) = data;
    }
    else
    if( function==1 )       // PIIX4 function 1
    {
        *(int *)(((BYTE *)&PCI_7111) + index) = data;
    }
    else
    {
        ASSERT(0);          // Invalid function device
    }
}


//-----------------------------------------------------------------------------
// Port I/O
//-----------------------------------------------------------------------------
static DWORD Piix4_fnIN(DWORD port, DWORD io_len)
{
    BYTE value = 0;
    TRACE(("Piix4_fnIN(port=%04X, io_len=%X)", port, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        case 0x61:
                Pix.port61 ^= 0x10;
                value = Pix.port61;
            break;

        default:    ASSERT(0);
    }

    TRACE(("Returns: %02X", value));
    return( value );
}


static void Piix4_fnOUT(DWORD port, DWORD data, DWORD io_len)
{
    TRACE(("Piix4_fnOUT(port=%04X, data=%X, io_len=%X)", port, data, io_len));
    ASSERT(io_len==1);

    switch( port )
    {
        case 0x61:
                Pix.port61 = (BYTE) data;
            break;

        default:    ASSERT(0);
    }
}
