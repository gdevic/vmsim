/******************************************************************************
*                                                                             *
*   Module:     PCI.c														  *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       4/19/2000													  *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This module contains the code for virtualizing a PCI configuration
        ports.

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/19/2000	 Original                                             Goran Devic *
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

DWORD PCI_AddressPort;

/******************************************************************************
*                                                                             *
*   Local Defines, Variables and Macros                                       *
*                                                                             *
******************************************************************************/

typedef struct
{
    DWORD res1          :2;     // Reserved - must be 0
    DWORD index         :6;     // Target doubleword (one of 64)
    DWORD function      :3;     // Target function number (one of 8)
    DWORD device        :5;     // Physical device number (one of 32)
    DWORD bus           :8;     // Target bus number (one of 256)
    DWORD res2          :7;     // Reserved - must be 0
    DWORD enable        :1;     // Enable translations

} TPCIAddress;

#define GET_PCI_ADDRESS_DEVICE    ((TPCIAddress*)&PCI_AddressPort)->device
#define GET_PCI_ADDRESS_FUNCTION  ((TPCIAddress*)&PCI_AddressPort)->function
#define GET_PCI_ADDRESS_INDEX     ((TPCIAddress*)&PCI_AddressPort)->index


/******************************************************************************
*                                                                             *
*   Functions                                                                 *
*                                                                             *
******************************************************************************/

static int PciInitialize(void);

static DWORD  PciDefaultIN(DWORD port, DWORD io_len);
static void   PciDefaultOUT(DWORD port, DWORD data, DWORD io_len);
static DWORD  PciAddressInCallback(DWORD port, DWORD io_len);
static void   PciAddressOutCallback(DWORD port, DWORD data, DWORD io_len);
static DWORD  PciDataInCallback(DWORD port, DWORD io_len);
static void   PciDataOutCallback(DWORD port, DWORD data, DWORD io_len);


/******************************************************************************
*                                                                             *
*   void PciRegister(TVDD *pVdd)                                              *
*                                                                             *
*******************************************************************************
*
*   Registration function for the PCI subsystem.
*
*   Where:
*       pVdd is the address of the virtual device desctiptor to fill in
*
******************************************************************************/
void PciRegister(TVDD *pVdd)
{
    pVdd->Initialize = PciInitialize;
    pVdd->Name = "PCI";
}


/******************************************************************************
*                                                                             *
*   int PciInitialize()                                                       *
*                                                                             *
*******************************************************************************
*
*   Initialization function for the PCI subsystem.
*
*   Returns:
*       0 on success
*       nonzero on fail
*
******************************************************************************/
static int PciInitialize()
{
    int i;
    TRACE(("PciInitialize()"));

    ASSERT(sizeof(TPciHeader)==256);

    // PCI can address 32 different devices. We use a direct index into an array
    // that specifies pointers to functions to device in/out handlers

    ASSERT(MAX_PCI_DEVICES==32);

    // Set the PCI config structures to point to our stubs since in the
    // dispatcher we dont check for valid pointers

    for( i=0; i<MAX_PCI_DEVICES; i++ )
    {
        Device.PCI[i].pPciInCallback  = PciDefaultIN;
        Device.PCI[i].pPciOutCallback = PciDefaultOUT;
    }

    // Register PCI configuration ports: address and data

    RegisterIOPort( 0x0CF8, 0x0CFB, PciAddressInCallback, PciAddressOutCallback );
    RegisterIOPort( 0x0CFC, 0x0CFF, PciDataInCallback, PciDataOutCallback );

    PCI_AddressPort = 0;

    return( 0 );
}


/******************************************************************************
*                                                                             *
*   int RegisterPCIDevice(DWORD device_number,                                *
*                         TPCI_IN_Callback fnIN, TPCI_OUT_Callback fnOUT )    *
*                                                                             *
*******************************************************************************
*
*   This function registers a PCI device with the PCI configuration manager.
*
*   Where:
*       device_number is a PCI device number, 0 - 31
*       fnIN is the function callback when reading from a config space
*       fnOUT is the function callback when writing to a config space
*
*   Returns:
*       0 if the registration succeeds
*       non-zero if the devicce can not be registered:
*           -1 : device is already registered
*           -2 : bad parameters to a call
*
******************************************************************************/
int RegisterPCIDevice(DWORD device_number, TPCI_IN_Callback fnIN, TPCI_OUT_Callback fnOUT )
{
    ASSERT(device_number < MAX_PCI_DEVICES);

    // Find the first empty PCI slot in our array

    if( device_number < MAX_PCI_DEVICES )
    {
        if( Device.PCI[device_number].fRegistered==0 )
        {
            // Found an empty slot - register it and store pointers.

            Device.PCI[device_number].fRegistered = 1;

            if( fnIN )  Device.PCI[device_number].pPciInCallback  = fnIN;
            if( fnOUT ) Device.PCI[device_number].pPciOutCallback = fnOUT;

            return( 0 );
        }
        return( -1 );
    }
    return( -2 );
}


//*****************************************************************************
//
//      Default function callbacks that are used when user sends NULL
//      in the place of a registered callback function.
//
//*****************************************************************************

static DWORD PciDefaultIN(DWORD port, DWORD io_len)
{
    ASSERT(0);
    return( 0 );
}

static void PciDefaultOUT(DWORD port, DWORD data, DWORD io_len)
{
    ASSERT(0);
    return;
}

//*****************************************************************************
//
//      CALLBACK FUNCTIONS
//
//*****************************************************************************

static DWORD PciAddressInCallback(DWORD port, DWORD io_len)
{
    if( io_len != 4 )
        DbgPrint(("io_len is not 4 (%d)", io_len));

    return( PCI_AddressPort );
}

static void PciAddressOutCallback(DWORD port, DWORD data, DWORD io_len)
{
    if( io_len != 4 )
        DbgPrint(("io_len is not 4 (%d)", io_len));

    PCI_AddressPort = data;

#if DBG
    {
        TPCIAddress *pAddress = (TPCIAddress *)&data;

        ASSERT(pAddress->res1==0);
        ASSERT(pAddress->res2==0);
        ASSERT(pAddress->bus==0);
        ASSERT(pAddress->enable==1);

        DbgPrint(("PCI address index: %d (0x%0X2)\n",pAddress->index,pAddress->index ));
        DbgPrint(("            function: %d\n",pAddress->function ));
        DbgPrint(("            device: %d\n",pAddress->device ));
        DbgPrint(("            enable: %d\n",pAddress->enable ));
    }
#endif
}

static DWORD PciDataInCallback(DWORD port, DWORD io_len)
{
    return( (*Device.PCI[GET_PCI_ADDRESS_DEVICE].pPciInCallback)(GET_PCI_ADDRESS_FUNCTION, GET_PCI_ADDRESS_INDEX) );
}

static void PciDataOutCallback(DWORD port, DWORD data, DWORD io_len)
{
    (*Device.PCI[GET_PCI_ADDRESS_DEVICE].pPciOutCallback)(GET_PCI_ADDRESS_FUNCTION, GET_PCI_ADDRESS_INDEX, data);
}

